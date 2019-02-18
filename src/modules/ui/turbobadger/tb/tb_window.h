/**
 * @file
 */

#pragma once

#include "tb_widgets_common.h"
#include "tb_widgets_listener.h"

namespace tb {

enum WINDOW_SETTINGS {
	WINDOW_SETTINGS_NONE = 0,		  ///< Chrome less window without any other settings.
	WINDOW_SETTINGS_TITLEBAR = 1,	 ///< Show a title bar that can also move the window.
	WINDOW_SETTINGS_RESIZABLE = 2,	///< Show a widget for resizing the window.
	WINDOW_SETTINGS_CLOSE_BUTTON = 4, ///< Show a widget for closing the window.
	WINDOW_SETTINGS_CAN_ACTIVATE = 8, ///< Can be activated and deactivate other windows.

	WINDOW_SETTINGS_DEFAULT = WINDOW_SETTINGS_TITLEBAR | WINDOW_SETTINGS_RESIZABLE | WINDOW_SETTINGS_CLOSE_BUTTON |
							  WINDOW_SETTINGS_CAN_ACTIVATE
};
MAKE_ENUM_FLAG_COMBO(WINDOW_SETTINGS);

/** TBWindow is a TBWidget that provides some window-like features.

	It can have a titlebar, be movable, resizable etc.

	It will activate and deactivate other windows on click (which will restore
	focus to the last focused child widget). */

class TBWindow : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBWindow, TBWidget);

	TBWindow();
	virtual ~TBWindow();

	/** Close this window.
		Warning: This window will be deleted after this call! */
	void close();

	/** Return true if this window is active. */
	bool isActive() const;

	/** Activate this window if it's not already activated.
		This will deactivate any currently activated window.
		This will automatically call EnsureFocus to restore/set focus to this window. */
	void activate();

	/** Ensure that this window has focus by attempting to find a focusable child widget.
		It will first try to restore focus to the last focused widget in this window,
		or a widget that has received SetFocus while the window was inactive.
		If that doesn't succeed, it will go through all children and try to set focus.
		Returns false if no focusable child was found. */
	bool ensureFocus();

	/** Set the widget that should be focused when this window is activated next time.
		This should not be used to change focus. Call TBWidget::setFocus to focus, which
		will call this method if the window is inactive! */
	void setLastFocus(TBWidget *last_focus) {
		m_last_focus.set(last_focus);
	}

	/** Set settings for how this window should look and behave. */
	void setSettings(WINDOW_SETTINGS settings);
	WINDOW_SETTINGS getSettings() {
		return m_settings;
	}

	/** RESIZE_FIT specifies how ResizeToFitContent should resize the window. */
	enum RESIZE_FIT {
		RESIZE_FIT_PREFERRED,		 ///< Fit the preferred size of all content
		RESIZE_FIT_MinIMAL,			 ///< Fit the minimal size of all content
		RESIZE_FIT_CURRENT_OR_NEEDED ///< Fit the minimal or maximum size only if needed. Will keep
									 ///< the new size as close as possible to the current size.
	};

	/** Get a suitable rect for the window based on the contents and the given fit. */
	TBRect getResizeToFitContentRect(RESIZE_FIT fit = RESIZE_FIT_PREFERRED);

	/** Resize the window to fit the its content. This is the same as doing
		SetRect(GetResizeToFitContentRect(fit)). */
	void resizeToFitContent(RESIZE_FIT fit = RESIZE_FIT_PREFERRED);

	/** Set the window title. */
	virtual bool setText(const char *text) override {
		return m_textfield.setText(text);
	}
	virtual bool getText(TBStr &text) override {
		return m_textfield.getText(text);
	}
	using TBWidget::getText; ///< Make all versions in base class available.

	/** Get the height of the title bar (or 0 if the WINDOW_SETTINGS say this window
		shouldn't have any title bar) */
	int getTitleHeight();

	virtual TBRect getPaddingRect() override;
	virtual PreferredSize onCalculatePreferredSize(const SizeConstraints &constraints) override;

	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onAdded() override;
	virtual void onRemove() override;
	virtual void onChildAdded(TBWidget *child) override;
	virtual void onResized(int old_w, int old_h) override;

protected:
	TBMover m_mover;
	TBResizer m_resizer;
	TBTextField m_textfield;
	TBWidget m_close_button;
	WINDOW_SETTINGS m_settings;
	TBWidgetSafePointer m_last_focus;
	TBWindow *getTopMostOtherWindow(bool only_activable_windows);
	void setWindowActiveState(bool active);
	void deActivate();
};

} // namespace tb
