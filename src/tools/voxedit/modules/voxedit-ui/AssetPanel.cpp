/**
 * @file
 */

#include "AssetPanel.h"
#include "DragAndDropPayload.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "video/Texture.h"
#include "video/gl/GLTypes.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/brush/StampBrush.h"
#include "voxelformat/VolumeFormat.h"

namespace voxedit {

AssetPanel::AssetPanel(ui::IMGUIApp *app) : Super(app) {
	loadTextures(_app->filesystem()->specialDir(io::FilesystemDirectories::FS_Dir_Pictures));
	loadModels(_app->filesystem()->specialDir(io::FilesystemDirectories::FS_Dir_Documents));
}

void AssetPanel::loadModels(const core::String &dir) {
	core::DynamicArray<io::FilesystemEntry> entities;
	_app->filesystem()->list(dir, entities);
	_models.clear();
	for (const auto &e : entities) {
		const core::String &fullName = core::string::path(dir, e.name);
		if (voxelformat::isModelFormat(fullName)) {
			_models.push_back(fullName);
		}
	}
}

void AssetPanel::loadTextures(const core::String &dir) {
	core::DynamicArray<io::FilesystemEntry> entities;
	_app->filesystem()->list(dir, entities);
	for (const auto &e : entities) {
		const core::String &fullName = core::string::path(dir, e.name);
		if (io::isImage(fullName)) {
			_texturePool.load(fullName, false);
		}
	}
}

void AssetPanel::update(const char *title, bool sceneMode, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		core_trace_scoped(AssetPanel);

		if (ImGui::CollapsingHeader(_("Models"), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::IconButton(ICON_LC_FOLDER_TREE, _("Open model directory"))) {
				auto callback = [this](const core::String &dir, const io::FormatDescription *desc) { loadModels(dir); };
				_app->directoryDialog(callback, {});
			}

			if (ImGui::BeginListBox("##assetmodels", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
				int n = 0;
				for (const core::String &model : _models) {
					const core::String &fileName = core::string::extractFilenameWithExtension(model);
					const core::String &label = core::String::format("%s##asset", fileName.c_str());
					const bool isSelected = (_currentSelectedModel == n);
					ImGui::Selectable(label.c_str(), isSelected);
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
					if (ImGui::BeginPopupContextItem()) {
						SceneManager &mgr = sceneMgr();
						if (ImGui::MenuItem(_("Use stamp"))) {
							Modifier &modifier = mgr.modifier();
							StampBrush &brush = modifier.stampBrush();
							if (brush.load(model)) {
								modifier.setBrushType(BrushType::Stamp);
							}
						}
						ImGui::TooltipText(_("This is only possible if the model doesn't exceed the max allowed stamp size"));
						if (ImGui::MenuItem(_("Add to scene"))) {
							mgr.import(model);
						}
						ImGui::EndPopup();
					}
					// TODO: load file - check for unsaved changes
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						ImGui::TextUnformatted(model.c_str());
						ImGui::SetDragDropPayload(dragdrop::ModelPayload, &model, sizeof(core::String),
												  ImGuiCond_Always);
						ImGui::EndDragDropSource();
					}
					++n;
				}
				ImGui::EndListBox();
			}
		}
		if (ImGui::CollapsingHeader(_("Images"), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::IconButton(ICON_LC_FOLDER_TREE, _("Open image directory"))) {
				auto callback = [this](const core::String &dir, const io::FormatDescription *desc) { loadTextures(dir); };
				_app->directoryDialog(callback, {});
			}
			int n = 1;
			ImGuiStyle &style = ImGui::GetStyle();
			const int maxImages = core_max(1, ImGui::GetWindowSize().x / (50 + style.ItemSpacing.x) - 1);
			for (const auto &e : _texturePool.cache()) {
				if (!e->second || !e->second->isLoaded()) {
					continue;
				}
				const video::Id handle = e->second->handle();
				const image::ImagePtr &image = _texturePool.loadImage(e->first);
				ImGui::ImageButton(handle, ImVec2(50, 50));
				ImGui::TooltipText("%s: %i:%i", image->name().c_str(), image->width(), image->height());
				if (n % maxImages == 0) {
					ImGui::NewLine();
				} else {
					ImGui::SameLine();
				}
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
					ImGui::ImageButton(handle, ImVec2(50, 50));
					ImGui::SetDragDropPayload(dragdrop::ImagePayload, (const void *)&image, sizeof(image),
											  ImGuiCond_Always);
					ImGui::EndDragDropSource();
				}
				++n;
			}
		}
	}
	ImGui::End();
}

} // namespace voxedit
