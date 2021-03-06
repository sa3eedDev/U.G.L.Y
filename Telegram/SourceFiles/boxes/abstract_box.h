/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "window/layer_widget.h"
#include "base/unique_qptr.h"
#include "ui/rp_widget.h"

namespace style {
struct RoundButton;
struct IconButton;
struct ScrollArea;
} // namespace style

namespace Ui {
class RoundButton;
class IconButton;
class ScrollArea;
class FlatLabel;
class FadeShadow;
} // namespace Ui

class BoxContent;

class BoxContentDelegate {
public:
	virtual void setLayerType(bool layerType) = 0;
	virtual void setTitle(rpl::producer<TextWithEntities> title) = 0;
	virtual void setAdditionalTitle(rpl::producer<QString> additional) = 0;
	virtual void setCloseByOutsideClick(bool close) = 0;

	virtual void clearButtons() = 0;
	virtual QPointer<Ui::RoundButton> addButton(
		rpl::producer<QString> text,
		Fn<void()> clickCallback,
		const style::RoundButton &st) = 0;
	virtual QPointer<Ui::RoundButton> addLeftButton(
		rpl::producer<QString> text,
		Fn<void()> clickCallback,
		const style::RoundButton &st) = 0;
	virtual QPointer<Ui::IconButton> addTopButton(
		const style::IconButton &st,
		Fn<void()> clickCallback) = 0;
	virtual void showLoading(bool show) = 0;
	virtual void updateButtonsPositions() = 0;

	virtual void showBox(
		object_ptr<BoxContent> box,
		LayerOptions options,
		anim::type animated) = 0;
	virtual void setDimensions(
		int newWidth,
		int maxHeight,
		bool forceCenterPosition = false) = 0;
	virtual void setNoContentMargin(bool noContentMargin) = 0;
	virtual bool isBoxShown() const = 0;
	virtual void closeBox() = 0;

	template <typename BoxType>
	QPointer<BoxType> show(
			object_ptr<BoxType> content,
			LayerOptions options = LayerOption::KeepOther,
			anim::type animated = anim::type::normal) {
		auto result = QPointer<BoxType>(content.data());
		showBox(std::move(content), options, animated);
		return result;
	}

	virtual QPointer<QWidget> outerContainer() = 0;

};

class BoxContent : public Ui::RpWidget, protected base::Subscriber {
	Q_OBJECT

public:
	BoxContent() {
		setAttribute(Qt::WA_OpaquePaintEvent);
	}

	bool isBoxShown() const {
		return getDelegate()->isBoxShown();
	}
	void closeBox() {
		getDelegate()->closeBox();
	}

	void setTitle(rpl::producer<QString> title);
	void setTitle(rpl::producer<TextWithEntities> title) {
		getDelegate()->setTitle(std::move(title));
	}
	void setAdditionalTitle(rpl::producer<QString> additional) {
		getDelegate()->setAdditionalTitle(std::move(additional));
	}
	void setCloseByEscape(bool close) {
		_closeByEscape = close;
	}
	void setCloseByOutsideClick(bool close) {
		getDelegate()->setCloseByOutsideClick(close);
	}

	void scrollToWidget(not_null<QWidget*> widget);

	void clearButtons() {
		getDelegate()->clearButtons();
	}
	QPointer<Ui::RoundButton> addButton(
		rpl::producer<QString> text,
		Fn<void()> clickCallback = nullptr);
	QPointer<Ui::RoundButton> addLeftButton(
		rpl::producer<QString> text,
		Fn<void()> clickCallback = nullptr);
	QPointer<Ui::IconButton> addTopButton(
			const style::IconButton &st,
			Fn<void()> clickCallback = nullptr) {
		return getDelegate()->addTopButton(st, std::move(clickCallback));
	}
	QPointer<Ui::RoundButton> addButton(
			rpl::producer<QString> text,
			const style::RoundButton &st) {
		return getDelegate()->addButton(std::move(text), nullptr, st);
	}
	QPointer<Ui::RoundButton> addButton(
			rpl::producer<QString> text,
			Fn<void()> clickCallback,
			const style::RoundButton &st) {
		return getDelegate()->addButton(
			std::move(text),
			std::move(clickCallback),
			st);
	}
	void showLoading(bool show) {
		getDelegate()->showLoading(show);
	}
	void updateButtonsGeometry() {
		getDelegate()->updateButtonsPositions();
	}

	virtual void setInnerFocus() {
		setFocus();
	}

	rpl::producer<> boxClosing() const {
		return _boxClosingStream.events();
	}
	void notifyBoxClosing() {
		_boxClosingStream.fire({});
	}

	void setDelegate(not_null<BoxContentDelegate*> newDelegate) {
		_delegate = newDelegate;
		_preparing = true;
		prepare();
		finishPrepare();
	}
	not_null<BoxContentDelegate*> getDelegate() const {
		return _delegate;
	}

public slots:
	void onScrollToY(int top, int bottom = -1);

	void onDraggingScrollDelta(int delta);

protected:
	virtual void prepare() = 0;

	void setLayerType(bool layerType) {
		getDelegate()->setLayerType(layerType);
	}

	void setNoContentMargin(bool noContentMargin) {
		if (_noContentMargin != noContentMargin) {
			_noContentMargin = noContentMargin;
			setAttribute(Qt::WA_OpaquePaintEvent, !_noContentMargin);
		}
		getDelegate()->setNoContentMargin(noContentMargin);
	}
	void setDimensions(
			int newWidth,
			int maxHeight,
			bool forceCenterPosition = false) {
		getDelegate()->setDimensions(
			newWidth,
			maxHeight,
			forceCenterPosition);
	}
	void setDimensionsToContent(
		int newWidth,
		not_null<Ui::RpWidget*> content);
	void setInnerTopSkip(int topSkip, bool scrollBottomFixed = false);
	void setInnerBottomSkip(int bottomSkip);

	template <typename Widget>
	QPointer<Widget> setInnerWidget(
			object_ptr<Widget> inner,
			const style::ScrollArea &st,
			int topSkip = 0,
			int bottomSkip = 0) {
		auto result = QPointer<Widget>(inner.data());
		setInnerTopSkip(topSkip);
		setInnerBottomSkip(bottomSkip);
		setInner(std::move(inner), st);
		return result;
	}

	template <typename Widget>
	QPointer<Widget> setInnerWidget(
			object_ptr<Widget> inner,
			int topSkip = 0,
			int bottomSkip = 0) {
		auto result = QPointer<Widget>(inner.data());
		setInnerTopSkip(topSkip);
		setInnerBottomSkip(bottomSkip);
		setInner(std::move(inner));
		return result;
	}

	template <typename Widget>
	object_ptr<Widget> takeInnerWidget() {
		return static_object_cast<Widget>(doTakeInnerWidget());
	}

	void setInnerVisible(bool scrollAreaVisible);
	QPixmap grabInnerCache();

	void resizeEvent(QResizeEvent *e) override;
	void paintEvent(QPaintEvent *e) override;
	void keyPressEvent(QKeyEvent *e) override;

private slots:
	void onScroll();
	void onInnerResize();

	void onDraggingScrollTimer();

private:
	void finishPrepare();
	void finishScrollCreate();
	void setInner(object_ptr<TWidget> inner);
	void setInner(object_ptr<TWidget> inner, const style::ScrollArea &st);
	void updateScrollAreaGeometry();
	void updateInnerVisibleTopBottom();
	void updateShadowsVisibility();
	object_ptr<TWidget> doTakeInnerWidget();

	BoxContentDelegate *_delegate = nullptr;

	bool _preparing = false;
	bool _noContentMargin = false;
	bool _closeByEscape = true;
	int _innerTopSkip = 0;
	int _innerBottomSkip = 0;
	object_ptr<Ui::ScrollArea> _scroll = { nullptr };
	object_ptr<Ui::FadeShadow> _topShadow = { nullptr };
	object_ptr<Ui::FadeShadow> _bottomShadow = { nullptr };

	object_ptr<QTimer> _draggingScrollTimer = { nullptr };
	int _draggingScrollDelta = 0;

	rpl::event_stream<> _boxClosingStream;

};

class AbstractBox
	: public Window::LayerWidget
	, public BoxContentDelegate
	, protected base::Subscriber {
public:
	AbstractBox(
		not_null<Window::LayerStackWidget*> layer,
		object_ptr<BoxContent> content);
	~AbstractBox();

	void parentResized() override;

	void setLayerType(bool layerType) override;
	void setTitle(rpl::producer<TextWithEntities> title) override;
	void setAdditionalTitle(rpl::producer<QString> additional) override;
	void showBox(
		object_ptr<BoxContent> box,
		LayerOptions options,
		anim::type animated) override;

	void clearButtons() override;
	QPointer<Ui::RoundButton> addButton(
		rpl::producer<QString> text,
		Fn<void()> clickCallback,
		const style::RoundButton &st) override;
	QPointer<Ui::RoundButton> addLeftButton(
		rpl::producer<QString> text,
		Fn<void()> clickCallback,
		const style::RoundButton &st) override;
	QPointer<Ui::IconButton> addTopButton(
		const style::IconButton &st,
		Fn<void()> clickCallback) override;
	void showLoading(bool show) override;
	void updateButtonsPositions() override;
	QPointer<QWidget> outerContainer() override;

	void setDimensions(
		int newWidth,
		int maxHeight,
		bool forceCenterPosition = false) override;

	void setNoContentMargin(bool noContentMargin) override {
		if (_noContentMargin != noContentMargin) {
			_noContentMargin = noContentMargin;
			updateSize();
		}
	}

	bool isBoxShown() const override {
		return !isHidden();
	}
	void closeBox() override {
		closeLayer();
	}

	void setCloseByOutsideClick(bool close) override;
	bool closeByOutsideClick() const override;

protected:
	void keyPressEvent(QKeyEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
	void paintEvent(QPaintEvent *e) override;

	void doSetInnerFocus() override {
		_content->setInnerFocus();
	}
	void closeHook() override {
		_content->notifyBoxClosing();
	}

private:
	struct LoadingProgress;

	void paintAdditionalTitle(Painter &p);
	void updateTitlePosition();
	void refreshLang();

	[[nodiscard]] bool hasTitle() const;
	[[nodiscard]] int titleHeight() const;
	[[nodiscard]] int buttonsHeight() const;
	[[nodiscard]] int buttonsTop() const;
	[[nodiscard]] int contentTop() const;
	[[nodiscard]] int countFullHeight() const;
	[[nodiscard]] int countRealHeight() const;
	[[nodiscard]] QRect loadingRect() const;
	void updateSize();

	not_null<Window::LayerStackWidget*> _layer;
	int _fullHeight = 0;

	bool _noContentMargin = false;
	int _maxContentHeight = 0;
	object_ptr<BoxContent> _content;

	object_ptr<Ui::FlatLabel> _title = { nullptr };
	Fn<TextWithEntities()> _titleFactory;
	rpl::variable<QString> _additionalTitle;
	int _titleLeft = 0;
	int _titleTop = 0;
	bool _layerType = false;
	bool _closeByOutsideClick = true;

	std::vector<object_ptr<Ui::RoundButton>> _buttons;
	object_ptr<Ui::RoundButton> _leftButton = { nullptr };
	base::unique_qptr<Ui::IconButton> _topButton = { nullptr };
	std::unique_ptr<LoadingProgress> _loadingProgress;

};

class BoxContentDivider : public Ui::RpWidget {
public:
	BoxContentDivider(QWidget *parent);
	BoxContentDivider(QWidget *parent, int height);

protected:
	void paintEvent(QPaintEvent *e) override;

};

class BoxPointer {
public:
	BoxPointer() = default;
	BoxPointer(const BoxPointer &other) = default;
	BoxPointer(BoxPointer &&other) : _value(base::take(other._value)) {
	}
	BoxPointer &operator=(const BoxPointer &other) {
		if (_value != other._value) {
			destroy();
			_value = other._value;
		}
		return *this;
	}
	BoxPointer &operator=(BoxPointer &&other) {
		if (_value != other._value) {
			destroy();
			_value = base::take(other._value);
		}
		return *this;
	}
	BoxPointer &operator=(BoxContent *other) {
		if (_value != other) {
			destroy();
			_value = other;
		}
		return *this;
	}
	~BoxPointer() {
		destroy();
	}

private:
	void destroy() {
		if (const auto value = base::take(_value)) {
			value->closeBox();
		}
	}

	QPointer<BoxContent> _value;

};
