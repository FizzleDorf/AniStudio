#pragma once

#include "ViewTypes.hpp"
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <exception>
#include <typeinfo>

namespace GUI {

	class IWorkspace {
	public:
		IWorkspace() = default;
		virtual ~IWorkspace() = default;
		virtual void Erase(const WorkspaceID id) {}
		virtual void UpdateViews(const float deltaT) {}
		virtual void RenderViews() = 0;
		virtual size_t Size() const { return 0; }
		virtual void Clear() {}
		virtual std::vector<WorkspaceID> GetAllWorkspaceIDs() const { return {}; }
	};

	template <typename T>
	class Workspace : public IWorkspace {
	public:
		Workspace() {
			data.reserve(16);
			std::cout << "[ViewList<" << typeid(T).name() << ">] Created with initial capacity: " << data.capacity() << std::endl;
		}

		~Workspace() {
			std::cout << "[ViewList<" << typeid(T).name() << ">] Destructor called, size: " << data.size() << std::endl;
		}

		void Insert(T &&view) {
			std::cout << "[ViewList<" << typeid(T).name() << ">] Insert START - ViewID: " << view.GetID()
				<< ", Current size: " << data.size()
				<< ", Capacity: " << data.capacity() << std::endl;

			if (data.capacity() <= data.size()) {
				size_t newCapacity = std::max(data.size() * 2, size_t(16));
				data.reserve(newCapacity);
				std::cout << "[ViewList<" << typeid(T).name() << ">] Reserved capacity: " << newCapacity << std::endl;
			}

			WorkspaceID id = view.GetID();

			if (id == 0) {
				std::cerr << "[ViewList<" << typeid(T).name() << ">] ERROR: Invalid ViewID (0)" << std::endl;
				return;
			}

			std::cout << "[ViewList<" << typeid(T).name() << ">] Processing ViewID: " << id << std::endl;

			bool thisTypeExists = false;
			for (const auto& existingView : data) {
				if (existingView && existingView->GetID() == id) {
					thisTypeExists = true;
					break;
				}
			}

			if (thisTypeExists) {
				std::cout << "[ViewList<" << typeid(T).name() << ">] Component type " << typeid(T).name()
					<< " already exists on ViewListID: " << id << " - REPLACING" << std::endl;

				for (auto& existingView : data) {
					if (existingView && existingView->GetID() == id) {
						existingView = std::make_shared<T>(std::forward<T>(view));
						std::cout << "[ViewList<" << typeid(T).name() << ">] Replaced existing component" << std::endl;
						return;
					}
				}
			}

			try {
				auto newView = std::make_shared<T>(std::forward<T>(view));

				if (!newView) {
					std::cerr << "[ViewList<" << typeid(T).name() << ">] ERROR: Failed to create shared_ptr" << std::endl;
					return;
				}

				if (newView->GetID() != id) {
					std::cerr << "[ViewList<" << typeid(T).name() << ">] ERROR: ViewID changed during shared_ptr creation!" << std::endl;
					return;
				}

				data.push_back(newView);
				std::cout << "[ViewList<" << typeid(T).name() << ">] NEW COMPONENT ADDED! ViewListID: " << id
					<< ", Type: " << typeid(T).name()
					<< ", Total components of this type: " << data.size()
					<< ", New capacity: " << data.capacity() << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[ViewList<" << typeid(T).name() << ">] Exception creating shared_ptr: " << e.what() << std::endl;
				throw;
			}
			catch (...) {
				std::cerr << "[ViewList<" << typeid(T).name() << ">] Unknown exception creating shared_ptr" << std::endl;
				throw;
			}
		}

		T &Get(const WorkspaceID id) {
			if (id == 0) {
				throw std::runtime_error("[ViewList<" + std::string(typeid(T).name()) + ">] Invalid ViewID (0)");
			}

			auto view = std::find_if(data.begin(), data.end(),
				[id](const std::shared_ptr<T> &v) {
				return v && v->GetID() == id;
			});

			if (view == data.end()) {
				std::cerr << "[ViewList<" << typeid(T).name() << ">] View not found with ID: " << id << std::endl;
				std::cerr << "[ViewList<" << typeid(T).name() << ">] Available views: ";
				for (const auto& v : data) {
					if (v) {
						std::cerr << v->GetID() << " ";
					}
				}
				std::cerr << std::endl;

				throw std::runtime_error("[ViewList<" + std::string(typeid(T).name()) + ">] View doesn't exist with ID: " + std::to_string(id));
			}

			return *(*view);
		}

		void Erase(const WorkspaceID id) override final {
			if (id == 0) {
				std::cout << "[ViewList<" << typeid(T).name() << ">] Ignoring erase of invalid ViewID (0)" << std::endl;
				return;
			}

			std::cout << "[ViewList<" << typeid(T).name() << ">] Attempting to erase ViewID: " << id << std::endl;

			auto view = std::find_if(data.begin(), data.end(),
				[id](const std::shared_ptr<T> &v) {
				return v && v->GetID() == id;
			});

			if (view != data.end()) {
				try {
					std::cout << "[ViewList<" << typeid(T).name() << ">] Found view to erase, removing..." << std::endl;
					data.erase(view);
					std::cout << "[ViewList<" << typeid(T).name() 
						<< ">] View erased! ID: " << id
						<< ", Type ID: " << ViewType<T>()
						<< ", Remaining views: " << data.size() << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[ViewList<" << typeid(T).name() << ">] Exception during erase: " << e.what() << std::endl;
					throw;
				}
			}
			else {
				std::cout << "[ViewList<" << typeid(T).name() << ">] No view found with ID: " << id
					<< ", Type ID: " << ViewType<T>() << std::endl;
			}
		}

		void UpdateViews(const float deltaT) override {
			// This prevents iterator invalidation if views modify the container
			std::vector<std::shared_ptr<T>> viewsCopy;

			try {
				// Quick copy without holding any locks
				viewsCopy = data;
			}
			catch (const std::exception& e) {
				std::cerr << "[ViewList<" << typeid(T).name() << ">] Exception copying views for update: " << e.what() << std::endl;
				return;
			}

			// Now safely iterate over the copy
			for (auto& view : viewsCopy) {
				if (view) {
					try {
						view->Update(deltaT);
					}
					catch (const std::exception& e) {
						std::cerr << "[ViewList<" << typeid(T).name() << ">] Exception in view Update(): " << e.what() << std::endl;
					}
				}
			}
		}

		void RenderViews() override {
			std::vector<std::shared_ptr<T>> viewsCopy;

			try {
				viewsCopy = data;
			}
			catch (const std::exception& e) {
				std::cerr << "[ViewList<" << typeid(T).name() << ">] Exception copying views for render: " << e.what() << std::endl;
				return;
			}

			for (auto &view : viewsCopy) {
				if (view) {
					try {
						view->Render();
					}
					catch (const std::exception& e) {
						std::cerr << "[ViewList<" << typeid(T).name() << ">] Exception in RenderViews: " << e.what() << ": " << e.what() << std::endl;

						continue;
					}
					catch (...) {
						std::cerr << "[ViewList<" << typeid(T).name() << ">] Unknown exception in RenderViews" << std::endl;
						continue;
					}
				}
				else {
					std::cerr << "[ViewList<" << typeid(T).name() << ">] WARNING: Null view found in list" << std::endl;
				}
			}
		}

		// Additional utility methods
		size_t Size() const override {
			return data.size();
		}

		void Clear() override {
			std::cout << "[ViewList<" << typeid(T).name() << ">] Clearing all views (count: " << data.size() << ")" << std::endl;
			data.clear();
		}

		std::vector<WorkspaceID> GetAllWorkspaceIDs() const override {
			std::vector<WorkspaceID> ids;
			for (const auto& ws : data) {
				if (ws) {
					ids.push_back(ws->GetID());
				}
			}
			return ids;
		}

		bool Contains(const WorkspaceID id) const {
			if (id == 0) return false;

			for (const auto& ws : data) {
				if (ws && ws->GetID() == id) {
					return true;
				}
			}
			return false;
		}

		void Compact() {
			size_t originalSize = data.size();
			data.erase(
				std::remove_if(data.begin(), data.end(),
					[](const std::shared_ptr<T>& view) { return !view; }),
				data.end()
			);

			if (data.size() != originalSize) {
				std::cout << "[ViewList<" << typeid(T).name() << ">] Compacted: removed "
					<< (originalSize - data.size()) << " null views" << std::endl;
			}
		}

		void DebugPrint() const {
			std::cout << "[ViewList<" << typeid(T).name() << ">] Debug - Total views: " << data.size() << std::endl;
			for (size_t i = 0; i < data.size(); ++i) {
				if (data[i]) {
					std::cout << "  [" << i << "] ViewID: " << data[i]->GetID()
						<< ", ViewName: " << data[i]->viewName << std::endl;
				}
				else {
					std::cout << "  [" << i << "] NULL VIEW" << std::endl;
				}
			}
		}

		// Iterator support
		auto begin() { return data.begin(); }
		auto end() { return data.end(); }
		auto begin() const { return data.begin(); }
		auto end() const { return data.end(); }

		const std::vector<std::shared_ptr<T>>& GetData() const { return data; }

	private:
		std::vector<std::shared_ptr<T>> data;
	};

} // namespace GUI