/**
 * @file
 */

#include "voxelpathtracer/PathTracer.h"
#include "SDL_timer.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "core/Log.h"
#include "image/Image.h"
#include "io/FileStream.h"
#include "io/FormatDescription.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"

class PathTracerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

public:
	bool onInitApp() override {
		if (!Super::onInitApp()) {
			return false;
		}
		voxelformat::FormatConfig::init();
		voxel::getPalette().nippon();
		return true;
	}
};

TEST_F(PathTracerTest, testHMec) {
	const io::FilePtr &file = io::filesystem()->open("hmec.vxl");
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	io::FileDescription fileDesc;
	fileDesc.set(file->name());
	scenegraph::SceneGraph sceneGraph;
	voxelformat::LoadContext testLoadCtx;

	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, stream, sceneGraph, testLoadCtx))
		<< "Could not load " << file->name();

	voxelpathtracer::PathTracer pathTracer;
	int dimensions = 512;
	ASSERT_TRUE(pathTracer.start(sceneGraph, dimensions, 8));
	while (!pathTracer.update()) {
		SDL_Delay(100);
	}
	const image::ImagePtr &img = pathTracer.image();
	ASSERT_TRUE(img);
	ASSERT_TRUE(img->isLoaded());
	ASSERT_EQ(dimensions, img->width());
	// ASSERT_EQ(dimensions, img->height());
	image::writeImage(img, file->name() + ".png");
	ASSERT_TRUE(pathTracer.stop());
}
