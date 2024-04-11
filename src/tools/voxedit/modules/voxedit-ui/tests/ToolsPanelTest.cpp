/**
 * @file
 */

#include "../ToolsPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void ToolsPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
#if 0
	ImGuiTest *test = IM_REGISTER_TEST(engine, testCategory(), testName());
	test->TestFunc = [=](ImGuiTestContext *ctx) {
		ctx->SetRef(title);
		IM_CHECK(focusWindow(ctx, title));
	};
#endif
}

} // namespace voxedit
