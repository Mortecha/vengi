/**
 * @file
 */

#pragma once

#include "animation/tb_animation.h"
#include "tb_widgets_listener.h"

namespace tb {

/** Don't use 0.0 for opacity animations since that may break focus code.
	At the moment a window should appear and start fading in from opacity 0,
	it would also attempt setting the focus to it, but if opacity is 0 it will
	think focus should not be set in that window and fail. */
#define TB_ALMOST_ZERO_OPACITY 0.001f

/** Base class for widget animations. This animation object will
	be deleted automatically if the widget is deleted. */
class TBWidgetAnimationObject : public TBAnimationObject, public TBLinkOf<TBWidgetAnimationObject> {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBWidgetAnimationObject, TBAnimationObject);

	TBWidgetAnimationObject(TBWidget *widget);
	virtual ~TBWidgetAnimationObject();

public:
	TBWidget *m_widget;
};

/** Animate the opacity of the target widget. */
class TBWidgetAnimationOpacity : public TBWidgetAnimationObject {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBWidgetAnimationOpacity, TBWidgetAnimationObject);

	TBWidgetAnimationOpacity(TBWidget *widget, float src_opacity, float dst_opacity, bool die);
	virtual void onAnimationStart() override;
	virtual void onAnimationUpdate(float progress) override;
	virtual void onAnimationStop(bool aborted) override;

private:
	float m_src_opacity;
	float m_dst_opacity;
	bool m_die;
};

/** Animate the rectangle of the target widget. */
class TBWidgetAnimationRect : public TBWidgetAnimationObject {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBWidgetAnimationRect, TBWidgetAnimationObject);

	enum MODE {
		/** Animate from source to dest. */
		MODE_SRC_TO_DST,
		/** Animate from current + delta to current. */
		MODE_DELTA_IN,
		/** Animate from current to current + delta. */
		MODE_DELTA_OUT
	};
	/** Animate the widget between the given source and dest rectangle. */
	TBWidgetAnimationRect(TBWidget *widget, const TBRect &src_rect, const TBRect &dst_rect);
	/** Animate the widget between rectangles based on the current widget
		rectangle and a delta. The reference rectangle will be taken from
		the target widget on the first OnAnimationUpdate. */
	TBWidgetAnimationRect(TBWidget *widget, const TBRect &delta_rect, MODE mode);
	virtual void onAnimationStart() override;
	virtual void onAnimationUpdate(float progress) override;
	virtual void onAnimationStop(bool aborted) override;

private:
	TBRect m_src_rect;
	TBRect m_dst_rect;
	TBRect m_delta_rect;
	MODE m_mode;
};

class TBWidgetsAnimationManager : public TBWidgetListener {
public:
	virtual ~TBWidgetsAnimationManager() {
	}
	/** Init the widgets animation manager. */
	static void init();

	/** Shutdown the widgets animation manager. */
	static void shutdown();

	/** Abort all animations that are running for the given widget. */
	static void abortAnimations(TBWidget *widget);

	/** Abort all animations matching the given type that are running for the given widget.
		This example will abort all opacity animations:
			AbortAnimations(widget, TBTypedObject::getTypeId<TBWidgetAnimationOpacity>()) */
	static void abortAnimations(TBWidget *widget, TB_TYPE_ID type_id);

private:
	// == TBWidgetListener ==================
	virtual void onWidgetDelete(TBWidget *widget) override;
	virtual bool onWidgetDying(TBWidget *widget) override;
	virtual void onWidgetAdded(TBWidget *parent, TBWidget *child) override;
	virtual void onWidgetRemove(TBWidget *parent, TBWidget *child) override;
};

} // namespace tb
