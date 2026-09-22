#pragma once

#include "ViewTypes.hpp"
#include "Log.hpp"
#include <memory>
#include <vector>
#include <string>
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
			ANI_LOG_DEBUG("[ViewList<%s>] Created with initial capacity: %zu",
				typeid(T).name(), data.capacity());
		}

		~Workspace() {
			ANI_LOG_DEBUG("[ViewList<%s>] Destructor called, size: %zu",
				typeid(T).name(), data.size());
		}

		void Insert(T&& view) {
			ANI_LOG_TRACE("[ViewList<%s>] Insert START - ViewID: %zu, Current size: %zu, Capacity: %zu",
				typeid(T).name(), (size_t)view.GetID(), data.size(), data.capacity());

			if (data.capacity() <= data.size()) {
				size_t newCapacity = std::max(data.size() * 2, size_t(16));
				data.reserve(newCapacity);
				ANI_LOG_TRACE("[ViewList<%s>] Reserved capacity: %zu",
					typeid(T).name(), newCapacity);
			}

			WorkspaceID id = view.GetID();

			if (id == 0) {
				ANI_LOG_WARN("[ViewList<%s>] Invalid ViewID (0), skipping insert",
					typeid(T).name());
				return;
			}

			ANI_LOG_TRACE("[ViewList<%s>] Processing ViewID: %zu", typeid(T).name(), (size_t)id);

			bool thisTypeExists = false;
			for (const auto& existingView : data) {
				if (existingView && existingView->GetID() == id) {
					thisTypeExists = true;
					break;
				}
			}

			if (thisTypeExists) {
				ANI_LOG_TRACE("[ViewList<%s>] Type %s already exists at ViewListID: %zu - REPLACING",
					typeid(T).name(), typeid(T).name(), (size_t)id);

				for (auto& existingView : data) {
					if (existingView && existingView->GetID() == id) {
						existingView = std::make_shared<T>(std::forward<T>(view));
						ANI_LOG_TRACE("[ViewList<%s>] Replaced existing view",
							typeid(T).name());
						return;
					}
				}
			}

			try {
				auto newView = std::make_shared<T>(std::forward<T>(view));

				if (!newView) {
					ANI_LOG_ERROR("[ViewList<%s>] Failed to create shared_ptr",
						typeid(T).name());
					return;
				}

				if (newView->GetID() != id) {
					ANI_LOG_ERROR("[ViewList<%s>] ViewID changed during shared_ptr creation!",
						typeid(T).name());
					return;
				}

				data.push_back(newView);
				ANI_LOG_DEBUG("[ViewList<%s>] NEW VIEW ADDED! ViewListID: %zu, Total views of this type: %zu, New capacity: %zu",
					typeid(T).name(), (size_t)id, data.size(), data.capacity());
			}
			catch (const std::exception& e) {
				ANI_LOG_ERROR("[ViewList<%s>] Exception creating shared_ptr: %s",
					typeid(T).name(), e.what());
				throw;
			}
			catch (...) {
				ANI_LOG_ERROR("[ViewList<%s>] Unknown exception creating shared_ptr",
					typeid(T).name());
				throw;
			}
		}

		T& Get(const WorkspaceID id) {
			if (id == 0) {
				throw std::runtime_error("[ViewList<" + std::string(typeid(T).name()) + ">] Invalid ViewID (0)");
			}

			auto view = std::find_if(data.begin(), data.end(),
				[id](const std::shared_ptr<T>& v) {
					return v && v->GetID() == id;
				});

			if (view == data.end()) {
				ANI_LOG_WARN("[ViewList<%s>] View not found with ID: %zu",
					typeid(T).name(), (size_t)id);

				// Build the list of available IDs into a string for one clean log line
				std::string availableIds;
				for (const auto& v : data) {
					if (v) {
						if (!availableIds.empty()) availableIds += " ";
						availableIds += std::to_string(v->GetID());
					}
				}
				ANI_LOG_WARN("[ViewList<%s>] Available view IDs: [%s]",
					typeid(T).name(), availableIds.c_str());

				throw std::runtime_error("[ViewList<" + std::string(typeid(T).name()) + ">] View doesn't exist with ID: " + std::to_string(id));
			}

			return *(*view);
		}

		void Erase(const WorkspaceID id) override final {
			if (id == 0) {
				ANI_LOG_TRACE("[ViewList<%s>] Ignoring erase of invalid ViewID (0)",
					typeid(T).name());
				return;
			}

			ANI_LOG_TRACE("[ViewList<%s>] Attempting to erase ViewID: %zu",
				typeid(T).name(), (size_t)id);

			auto view = std::find_if(data.begin(), data.end(),
				[id](const std::shared_ptr<T>& v) {
					return v && v->GetID() == id;
				});

			if (view != data.end()) {
				try {
					ANI_LOG_TRACE("[ViewList<%s>] Found view to erase, removing...",
						typeid(T).name());
					data.erase(view);
					ANI_LOG_DEBUG("[ViewList<%s>] View erased! ID: %zu, Type ID: %zu, Remaining views: %zu",
						typeid(T).name(), (size_t)id, (size_t)ViewType<T>(), data.size());
				}
				catch (const std::exception& e) {
					ANI_LOG_ERROR("[ViewList<%s>] Exception during erase: %s",
						typeid(T).name(), e.what());
					throw;
				}
			}
			else {
				ANI_LOG_TRACE("[ViewList<%s>] No view found with ID: %zu, Type ID: %zu",
					typeid(T).name(), (size_t)id, (size_t)ViewType<T>());
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
				ANI_LOG_ERROR("[ViewList<%s>] Exception copying views for update: %s",
					typeid(T).name(), e.what());
				return;
			}

			// Now safely iterate over the copy
			for (auto& view : viewsCopy) {
				if (view) {
					try {
						view->Update(deltaT);
					}
					catch (const std::exception& e) {
						ANI_LOG_ERROR("[ViewList<%s>] Exception in view Update(): %s",
							typeid(T).name(), e.what());
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
				ANI_LOG_ERROR("[ViewList<%s>] Exception copying views for render: %s",
					typeid(T).name(), e.what());
				return;
			}

			for (auto& view : viewsCopy) {
				if (view) {
					try {
						view->Render();
					}
					catch (const std::exception& e) {
						ANI_LOG_ERROR("[ViewList<%s>] Exception in RenderViews: %s",
							typeid(T).name(), e.what());
						continue;
					}
					catch (...) {
						ANI_LOG_ERROR("[ViewList<%s>] Unknown exception in RenderViews",
							typeid(T).name());
						continue;
					}
				}
				else {
					ANI_LOG_WARN("[ViewList<%s>] Null view found in list",
						typeid(T).name());
				}
			}
		}

		// Additional utility methods
		size_t Size() const override {
			return data.size();
		}

		void Clear() override {
			ANI_LOG_DEBUG("[ViewList<%s>] Clearing all views (count: %zu)",
				typeid(T).name(), data.size());
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
				ANI_LOG_DEBUG("[ViewList<%s>] Compacted: removed %zu null views",
					typeid(T).name(), originalSize - data.size());
			}
		}

		void DebugPrint() const {
			ANI_LOG_DEBUG("[ViewList<%s>] Debug - Total views: %zu",
				typeid(T).name(), data.size());
			for (size_t i = 0; i < data.size(); ++i) {
				if (data[i]) {
					ANI_LOG_TRACE("  [%zu] ViewID: %zu, ViewName: %s",
						i, (size_t)data[i]->GetID(), data[i]->viewName.c_str());
				}
				else {
					ANI_LOG_TRACE("  [%zu] NULL VIEW", i);
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