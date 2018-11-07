/***************************************************************************
 *   Copyright (C) 2017 by Nicolas Carion                                  *
 *   This file is part of Kdenlive. See www.kdenlive.org.                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3 or any later version accepted by the       *
 *   membership of KDE e.V. (or its successor approved  by the membership  *
 *   of KDE e.V.), which shall act as a proxy defined in Section 14 of     *
 *   version 3 of the license.                                             *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "assetpanel.hpp"
#include "core.cpp"
#include "definitions.h"
#include "effects/effectstack/model/effectitemmodel.hpp"
#include "effects/effectstack/model/effectstackmodel.hpp"
#include "effects/effectstack/view/effectstackview.hpp"
#include "kdenlivesettings.h"
#include "model/assetparametermodel.hpp"
#include "transitions/transitionsrepository.hpp"
#include "transitions/view/transitionstackview.hpp"

#include "view/assetparameterview.hpp"

#include <KColorScheme>
#include <KColorUtils>
#include <KSqueezedTextLabel>
#include <KDualAction>
#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <klocalizedstring.h>

AssetPanel::AssetPanel(QWidget *parent)
    : QWidget(parent)
    , m_lay(new QVBoxLayout(this))
    , m_assetTitle(new KSqueezedTextLabel(this))
    , m_container(new QWidget(this))
    , m_transitionWidget(new TransitionStackView(this))
    , m_effectStackWidget(new EffectStackView(this))
{
    QToolBar *buttonToolbar = new QToolBar(this);
    buttonToolbar->addWidget(m_assetTitle);
    int size = style()->pixelMetric(QStyle::PM_SmallIconSize);
    QSize iconSize(size, size);
    buttonToolbar->setIconSize(iconSize);

    // spacer
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Maximum);
    buttonToolbar->addWidget(empty);

    m_switchBuiltStack = new QToolButton(this);
    m_switchBuiltStack->setIcon(QIcon::fromTheme(QStringLiteral("adjustlevels")));
    m_switchBuiltStack->setToolTip(i18n("Adjust clip"));
    m_switchBuiltStack->setCheckable(true);
    m_switchBuiltStack->setChecked(KdenliveSettings::showbuiltstack());
    m_switchBuiltStack->setVisible(false);
    // connect(m_switchBuiltStack, &QToolButton::toggled, m_effectStackWidget, &EffectStackView::switchBuiltStack);
    buttonToolbar->addWidget(m_switchBuiltStack);

    m_splitButton = new KDualAction(i18n("Normal view"), i18n("Compare effect"), this);
    m_splitButton->setActiveIcon(QIcon::fromTheme(QStringLiteral("view-right-close")));
    m_splitButton->setInactiveIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    m_splitButton->setToolTip(i18n("Compare effect"));
    m_splitButton->setVisible(false);
    connect(m_splitButton, &KDualAction::activeChangedByUser, this, &AssetPanel::processSplitEffect);
    buttonToolbar->addAction(m_splitButton);

    m_enableStackButton = new KDualAction(i18n("Effects disabled"), i18n("Effects enabled"), this);
    m_enableStackButton->setInactiveIcon(QIcon::fromTheme(QStringLiteral("hint")));
    m_enableStackButton->setActiveIcon(QIcon::fromTheme(QStringLiteral("visibility")));
    connect(m_enableStackButton, &KDualAction::activeChangedByUser, this, &AssetPanel::enableStack);
    m_enableStackButton->setVisible(false);
    buttonToolbar->addAction(m_enableStackButton);

    m_timelineButton = new KDualAction(i18n("Hide keyframes"), i18n("Display keyframes in timeline"), this);
    m_timelineButton->setInactiveIcon(QIcon::fromTheme(QStringLiteral("adjustlevels")));
    m_timelineButton->setActiveIcon(QIcon::fromTheme(QStringLiteral("adjustlevels")));
    m_timelineButton->setToolTip(i18n("Display keyframes in timeline"));
    m_timelineButton->setVisible(false);
    connect(m_timelineButton, &KDualAction::activeChangedByUser, this, &AssetPanel::showKeyframes);
    buttonToolbar->addAction(m_timelineButton);

    m_lay->addWidget(buttonToolbar);
    m_lay->setContentsMargins(0, 0, 0, 0);
    m_lay->setSpacing(0);
    QVBoxLayout *lay = new QVBoxLayout(m_container);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_transitionWidget);
    lay->addWidget(m_effectStackWidget);
    QScrollArea *sc = new QScrollArea;
    sc->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sc->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sc->setFrameStyle(QFrame::NoFrame);
    sc->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding));
    m_container->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding));
    sc->setWidgetResizable(true);

    m_lay->addWidget(sc);
    sc->setWidget(m_container);
    m_transitionWidget->setVisible(false);
    m_effectStackWidget->setVisible(false);
    updatePalette();
    connect(m_effectStackWidget, &EffectStackView::seekToPos, this, &AssetPanel::seekToPos);
    connect(m_effectStackWidget, &EffectStackView::reloadEffect, this, &AssetPanel::reloadEffect);
    connect(m_transitionWidget, &TransitionStackView::seekToTransPos, this, &AssetPanel::seekToPos);
    connect(m_effectStackWidget, &EffectStackView::updateEnabledState, [this]() {
        m_enableStackButton->setActive(m_effectStackWidget->isStackEnabled());
    });
}

void AssetPanel::showTransition(int tid, std::shared_ptr<AssetParameterModel> transitionModel)
{
    Q_UNUSED(tid)
    clear();
    QString transitionId = transitionModel->getAssetId();
    QString transitionName = TransitionsRepository::get()->getName(transitionId);
    m_assetTitle->setText(i18n("%1 properties", transitionName));
    m_transitionWidget->setVisible(true);
    m_timelineButton->setVisible(true);
    m_enableStackButton->setVisible(false);
    m_transitionWidget->setModel(transitionModel, QSize(), true);
}

void AssetPanel::showEffectStack(const QString &itemName, std::shared_ptr<EffectStackModel> effectsModel, QSize frameSize, bool showKeyframes)
{
    m_splitButton->setActive(false);
    if (effectsModel == nullptr) {
        // Item is not ready
        m_splitButton->setVisible(false);
        m_enableStackButton->setVisible(false);
        clear();
        return;
    }
    ObjectId id = effectsModel->getOwnerId();
    if (m_effectStackWidget->stackOwner() == id) {
        // already on this effect stack, do nothing
        return;
    }
    clear();
    QString title;
    bool showSplit = false;
    bool enableKeyframes = false;
    switch (id.first) {
    case ObjectType::TimelineClip:
        title = i18n("%1 effects", itemName);
        showSplit = true;
        enableKeyframes = true;
        break;
    case ObjectType::TimelineComposition:
        title = i18n("%1 parameters", itemName);
        enableKeyframes = true;
        break;
    case ObjectType::TimelineTrack:
        title = i18n("Track %1 effects", itemName);
        // TODO: track keyframes
        // enableKeyframes = true;
        break;
    case ObjectType::BinClip:
        title = i18n("Bin %1 effects", itemName);
        showSplit = true;
        break;
    default:
        title = itemName;
        break;
    }
    m_assetTitle->setText(title);
    m_splitButton->setVisible(showSplit);
    m_enableStackButton->setVisible(id.first != ObjectType::TimelineComposition);
    m_enableStackButton->setActive(effectsModel->isStackEnabled());
    if (showSplit) {
        m_splitButton->setEnabled(effectsModel->rowCount() > 0);
        QObject::connect(effectsModel.get(), &EffectStackModel::dataChanged, [&](){
            if (m_effectStackWidget->isEmpty()) {
                m_splitButton->setActive(false);
            }
            m_splitButton->setEnabled(!m_effectStackWidget->isEmpty());
        });
    }
    m_timelineButton->setVisible(enableKeyframes);
    m_timelineButton->setActive(showKeyframes);
    // Disable built stack until properly implemented
    // m_switchBuiltStack->setVisible(true);
    m_effectStackWidget->setVisible(true);
    m_effectStackWidget->setModel(effectsModel, frameSize);
}

void AssetPanel::clearAssetPanel(int itemId)
{
    ObjectId id = m_effectStackWidget->stackOwner();
    if (id.first == ObjectType::TimelineClip && id.second == itemId) {
        clear();
    } else {
        id = m_transitionWidget->stackOwner();
        if (id.first == ObjectType::TimelineComposition && id.second == itemId) {
            clear();
        }
    }
}

void AssetPanel::clear()
{
    m_transitionWidget->setVisible(false);
    m_transitionWidget->unsetModel();
    m_effectStackWidget->setVisible(false);
    m_splitButton->setVisible(false);
    m_timelineButton->setVisible(false);
    m_switchBuiltStack->setVisible(false);
    m_effectStackWidget->unsetModel();
    m_assetTitle->setText(QString());
}

void AssetPanel::updatePalette()
{
    QString styleSheet = getStyleSheet();
    setStyleSheet(styleSheet);
    m_transitionWidget->setStyleSheet(styleSheet);
    m_effectStackWidget->setStyleSheet(styleSheet);
}

// static
const QString AssetPanel::getStyleSheet()
{
    KColorScheme scheme(QApplication::palette().currentColorGroup(), KColorScheme::View);
    QColor selected_bg = scheme.decoration(KColorScheme::FocusColor).color();
    QColor hgh = KColorUtils::mix(QApplication::palette().window().color(), selected_bg, 0.2);
    QColor hover_bg = scheme.decoration(KColorScheme::HoverColor).color();
    QColor light_bg = scheme.shade(KColorScheme::LightShade);
    QColor alt_bg = scheme.background(KColorScheme::NormalBackground).color();

    QString stylesheet;

    // effect background
    stylesheet.append(QStringLiteral("QFrame#decoframe {border-bottom:2px solid "
                                     "palette(mid);background: transparent} QFrame#decoframe[active=\"true\"] {background: %1;}")
                          .arg(hgh.name()));

    // effect in group background
    stylesheet.append(
        QStringLiteral("QFrame#decoframesub {border-top:1px solid palette(light);}  QFrame#decoframesub[active=\"true\"] {background: %1;}").arg(hgh.name()));

    // group background
    stylesheet.append(QStringLiteral("QFrame#decoframegroup {border:2px solid palette(dark);margin:0px;margin-top:2px;} "));

    // effect title bar
    stylesheet.append(QStringLiteral("QFrame#frame {margin-bottom:2px;}  QFrame#frame[target=\"true\"] "
                                     "{background: palette(highlight);}"));

    // group effect title bar
    stylesheet.append(QStringLiteral("QFrame#framegroup {background: palette(dark);}  "
                                     "QFrame#framegroup[target=\"true\"] {background: palette(highlight);} "));

    // draggable effect bar content
    stylesheet.append(QStringLiteral("QProgressBar::chunk:horizontal {background: palette(button);border-top-left-radius: 4px;border-bottom-left-radius: 4px;} "
                                     "QProgressBar::chunk:horizontal#dragOnly {background: %1;border-top-left-radius: 4px;border-bottom-left-radius: 4px;} "
                                     "QProgressBar::chunk:horizontal:hover {background: %2;}")
                          .arg(alt_bg.name(), selected_bg.name()));

    // draggable effect bar
    stylesheet.append(QStringLiteral("QProgressBar:horizontal {border: 1px solid palette(dark);border-top-left-radius: 4px;border-bottom-left-radius: "
                                     "4px;border-right:0px;background:%3;padding: 0px;text-align:left center} QProgressBar:horizontal:disabled {border: 1px "
                                     "solid palette(button)} QProgressBar:horizontal#dragOnly {background: %3} QProgressBar:horizontal[inTimeline=\"true\"] { "
                                     "border: 1px solid %1;border-right: 0px;background: %2;padding: 0px;text-align:left center } "
                                     "QProgressBar::chunk:horizontal[inTimeline=\"true\"] {background: %1;}")
                          .arg(hover_bg.name(), light_bg.name(), alt_bg.name()));

    // spin box for draggable widget
    stylesheet.append(
        QStringLiteral("QAbstractSpinBox#dragBox {border: 1px solid palette(dark);border-top-right-radius: 4px;border-bottom-right-radius: "
                       "4px;padding-right:0px;} QAbstractSpinBox::down-button#dragBox {width:0px;padding:0px;} QAbstractSpinBox:disabled#dragBox {border: 1px "
                       "solid palette(button);} QAbstractSpinBox::up-button#dragBox {width:0px;padding:0px;} QAbstractSpinBox[inTimeline=\"true\"]#dragBox { "
                       "border: 1px solid %1;} QAbstractSpinBox:hover#dragBox {border: 1px solid %2;} ")
            .arg(hover_bg.name(), selected_bg.name()));

    // group editable labels
    stylesheet.append(QStringLiteral("MyEditableLabel { background-color: transparent; color: palette(bright-text); border-radius: 2px;border: 1px solid "
                                     "transparent;} MyEditableLabel:hover {border: 1px solid palette(highlight);} "));

    // transparent qcombobox
    stylesheet.append(QStringLiteral("QComboBox { background-color: transparent;} "));

    return stylesheet;
}

void AssetPanel::processSplitEffect(bool enable)
{
    ObjectType id = m_effectStackWidget->stackOwner().first;
    if (id == ObjectType::TimelineClip) {
        emit doSplitEffect(enable);
    } else if (id == ObjectType::BinClip) {
        emit doSplitBinEffect(enable);
    }
}

void AssetPanel::showKeyframes(bool enable)
{
    if (m_transitionWidget->isVisible()) {
        pCore->showClipKeyframes(m_transitionWidget->stackOwner(), enable);
    } else {
        pCore->showClipKeyframes(m_effectStackWidget->stackOwner(), enable);
    }
}

ObjectId AssetPanel::effectStackOwner()
{
    if (m_transitionWidget->isVisible()) {
        return m_transitionWidget->stackOwner();
    }
    if (!m_effectStackWidget->isVisible()) {
        return ObjectId(ObjectType::NoItem, -1);
    }
    return m_effectStackWidget->stackOwner();
}

void AssetPanel::parameterChanged(QString name, int value)
{
    Q_UNUSED(name)
    emit changeSpeed(value);
}

bool AssetPanel::addEffect(const QString &effectId)
{
    if (!m_effectStackWidget->isVisible()) {
        return false;
    }
    return m_effectStackWidget->addEffect(effectId);
}

void AssetPanel::enableStack(bool enable)
{
    if (!m_effectStackWidget->isVisible()) {
        return;
    }
    m_effectStackWidget->enableStack(enable);
}