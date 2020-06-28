/**
 * @file
 */

#pragma once

#include "ui/turbobadger/TurboBadger.h"
#include "../../NoiseData.h"

class NoiseTool;

class NoiseItem: public tb::TBGenericStringItem {
public:
	NoiseItem(const char* str, const tb::TBID& id, const NoiseData& data) :
			tb::TBGenericStringItem(str, id), _data(data) {
	}
	const NoiseData& data() const {
		return _data;
	}

private:
	NoiseData _data;
};

class NoiseItemSource: public tb::TBSelectItemSourceList<NoiseItem> {
private:
	NoiseTool* _tool;

public:
	NoiseItemSource(NoiseTool* tool);

	virtual bool filter(int index, const char *filter);
	virtual tb::TBWidget *createItemWidget(int index, tb::TBSelectItemViewer *viewer);
};

class NoiseDataItemWidget: public tb::TBLayout {
private:
	using Super = tb::TBLayout;
	NoiseItemSource *_source;
	int _index;
	NoiseTool* _tool;
public:
	NoiseDataItemWidget(NoiseTool* tool, NoiseItem *item, NoiseItemSource *source, int index);

	bool onEvent(const tb::TBWidgetEvent &ev) override;
};

class NoiseDataList : public tb::TBLayout {
private:
	tb::TBSelectList* _select;
public:
	UIWIDGET_SUBCLASS(NoiseDataList, tb::TBLayout);

	NoiseDataList();
	~NoiseDataList();

	bool onEvent(const tb::TBWidgetEvent &ev) override;
};

UIWIDGET_FACTORY(NoiseDataList, tb::TBValue::TYPE_NULL, tb::WIDGET_Z_TOP)
