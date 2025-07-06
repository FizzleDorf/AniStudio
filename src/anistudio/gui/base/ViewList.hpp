/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

#pragma once

#include "ViewTypes.hpp"
#include "pch.h"

namespace GUI {

	class IViewList {
	public:
		IViewList() = default;
		virtual ~IViewList() = default;
		virtual void Erase(const ViewListID viewID) {}
		virtual void UpdateViews(const float deltaT) {}
		virtual void RenderViews() = 0;
		virtual size_t Size() const { return 0; }
		virtual void Clear() {}
		virtual std::vector<ViewListID> GetAllViewIDs() const { return {}; }
	};

	template <typename T>
	class ViewList : public IViewList {
	public:
		ViewList() {
			data.reserve(16);
			std::cout << "[ViewList<" << typeid(T).name() << ">] Created with initial capacity: " << data.capacity() << std::endl;
		}

		~ViewList() {
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

			ViewListID viewID = view.GetID();

			if (viewID == 0) {
				std::cerr << "[ViewList<" << typeid(T).name() << ">] ERROR: Invalid ViewID (0)" << std::endl;
				return;
			}

			std::cout << "[ViewList<" << typeid(T).name() << ">] Processing ViewID: " << viewID << std::endl;

			bool thisTypeExists = false;
			for (const auto& existingView : data) {
				if (existingView && existingView->GetID() == viewID) {
					thisTypeExists = true;
					break;
				}
			}

			if (thisTypeExists) {
				std::cout << "[ViewList<" << typeid(T).name() << ">] Component type " << typeid(T).name()
					<< " already exists on ViewListID: " << viewID << " - REPLACING" << std::endl;

				for (auto& existingView : data) {
					if (existingView && existingView->GetID() == viewID) {
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

				if (newView->GetID() != viewID) {
					std::cerr << "[ViewList<" << typeid(T).name() << ">] ERROR: ViewID changed during shared_ptr creation!" << std::endl;
					return;
				}

				data.push_back(newView);
				std::cout << "[ViewList<" << typeid(T).name() << ">] NEW COMPONENT ADDED! ViewListID: " << viewID
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

		T &Get(const ViewListID viewID) {
			if (viewID == 0) {
				throw std::runtime_error("[ViewList<" + std::string(typeid(T).name()) + ">] Invalid ViewID (0)");
			}

			auto view = std::find_if(data.begin(), data.end(),
				[viewID](const std::shared_ptr<T> &v) {
				return v && v->GetID() == viewID;
			});

			if (view == data.end()) {
				std::cerr << "[ViewList<" << typeid(T).name() << ">] View not found with ID: " << viewID << std::endl;
				std::cerr << "[ViewList<" << typeid(T).name() << ">] Available views: ";
				for (const auto& v : data) {
					if (v) {
						std::cerr << v->GetID() << " ";
					}
				}
				std::cerr << std::endl;

				throw std::runtime_error("[ViewList<" + std::string(typeid(T).name()) + ">] View doesn't exist with ID: " + std::to_string(viewID));
			}

			return *(*view);
		}

		void Erase(const ViewListID viewID) override final {
			if (viewID == 0) {
				std::cout << "[ViewList<" << typeid(T).name() << ">] Ignoring erase of invalid ViewID (0)" << std::endl;
				return;
			}

			std::cout << "[ViewList<" << typeid(T).name() << ">] Attempting to erase ViewID: " << viewID << std::endl;

			auto view = std::find_if(data.begin(), data.end(),
				[viewID](const std::shared_ptr<T> &v) {
				return v && v->GetID() == viewID;
			});

			if (view != data.end()) {
				try {
					std::cout << "[ViewList<" << typeid(T).name() << ">] Found view to erase, removing..." << std::endl;
					data.erase(view);
					std::cout << "[ViewList<" << typeid(T).name() << ">] View erased! ID: " << viewID
						<< ", Type ID: " << ViewType<T>()
						<< ", Remaining views: " << data.size() << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[ViewList<" << typeid(T).name() << ">] Exception during erase: " << e.what() << std::endl;
					throw;
				}
			}
			else {
				std::cout << "[ViewList<" << typeid(T).name() << ">] No view found with ID: " << viewID
					<< ", Type ID: " << ViewType<T>() << std::endl;
			}
		}

		void UpdateViews(const float deltaT) override {
			// CRITICAL FIX: Create a copy of the data to iterate over
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

		std::vector<ViewListID> GetAllViewIDs() const override {
			std::vector<ViewListID> viewIDs;
			for (const auto& view : data) {
				if (view) {
					viewIDs.push_back(view->GetID());
				}
			}
			return viewIDs;
		}

		bool Contains(const ViewListID viewID) const {
			if (viewID == 0) return false;

			for (const auto& view : data) {
				if (view && view->GetID() == viewID) {
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