/*
SPDX-FileCopyrightText: 2019 Jean-Baptiste Mardelle <jb@kdenlive.org>
This file is part of Kdenlive. See www.kdenlive.org.

SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "tagwidget.hpp"
#include "core.h"
#include "mainwindow.h"

#include <KActionCollection>
#include <KLocalizedString>

#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QDrag>
#include <QFontDatabase>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

DragButton::DragButton(int ix, const QString &tag, const QString &description, QWidget *parent)
    : QToolButton(parent)
    , m_tag(tag.toLower())
    , m_description(description)
    , m_dragging(false)
{
    setToolTip(description);
    int iconSize = QFontInfo(font()).pixelSize() - 2;
    QPixmap pix(iconSize, iconSize);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(QColor(m_tag));
    p.drawRoundedRect(0, 0, iconSize, iconSize, iconSize/2, iconSize/2);
    setAutoRaise(true);
    setText(description);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setCheckable(true);
    QAction *ac = new QAction(description.isEmpty() ? i18n("Tag %1", ix) : description, this);
    ac->setData(m_tag);
    ac->setIcon(QIcon(pix));
    ac->setCheckable(true);
    setDefaultAction(ac);
    pCore->window()->addAction(QString("tag_%1").arg(ix), ac, {}, QStringLiteral("bintags"));
    connect(ac, &QAction::triggered, this, [&] (bool checked) {
        emit switchTag(m_tag, checked);
    });
}

void DragButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPosition = event->pos();
    QToolButton::mousePressEvent(event);
    m_dragging = false;
}

void DragButton::mouseMoveEvent(QMouseEvent *event)
{
    QToolButton::mouseMoveEvent(event);
    if (!(event->buttons() & Qt::LeftButton) || m_dragging) {
        return;
    }
    if ((event->pos() - m_dragStartPosition).manhattanLength()
         < QApplication::startDragDistance()) {
        return;
    }

    auto *drag = new QDrag(this);
    auto *mimeData = new QMimeData;
    mimeData->setData(QStringLiteral("kdenlive/tag"), m_tag.toUtf8());
    drag->setPixmap(defaultAction()->icon().pixmap(22, 22));
    drag->setMimeData(mimeData);
    m_dragging = true;
    drag->exec(Qt::CopyAction | Qt::MoveAction);
    // Disable / enable toolbutton to clear highlighted state because mouserelease is not triggered on drop end
    setEnabled(false);
    setEnabled(true);
}

void DragButton::mouseReleaseEvent(QMouseEvent *event)
{
    QToolButton::mouseReleaseEvent(event);
    if ((event->button() == Qt::LeftButton) && !m_dragging) {
        emit switchTag(m_tag, isChecked());
    }
    m_dragging = false;
}

const QString &DragButton::tag() const
{
    return m_tag;
}

const QString &DragButton::description() const
{
    return m_description;
}

TagWidget::TagWidget(QWidget *parent)
    : QWidget(parent)
{
    setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    auto *lay = new QHBoxLayout;
    lay->setContentsMargins(2, 0, 2, 0);
    lay->addStretch(10);
    auto *config = new QToolButton(this);
    QAction *ca = new QAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("Configure"), this);
    config->setAutoRaise(true);
    config->setDefaultAction(ca);
    connect(config, &QToolButton::triggered, this, [&]() {
        showTagsConfig ();
    });
    lay->addWidget(config);
    setLayout(lay);
}

void TagWidget::setTagData(const QString &tagData)
{
    QStringList colors = tagData.toLower().split(QLatin1Char(';'));
    for (DragButton *tb : qAsConst(tags)) {
        const QString color = tb->tag();
        tb->defaultAction()->setChecked(colors.contains(color));
    }
}

void TagWidget::rebuildTags(const QMap <QString, QString> &newTags)
{
    auto *lay = static_cast<QHBoxLayout *>(layout());
    qDeleteAll(tags);
    tags.clear();
    int ix = 1;
    QMapIterator<QString, QString> i(newTags);
    while (i.hasNext()) {
        i.next();
        DragButton *tag1 = new DragButton(ix, i.key(), i.value(), this);
        tag1->setFont(font());
        connect(tag1, &DragButton::switchTag, this, &TagWidget::switchTag);
        tags << tag1;
        lay->insertWidget(ix - 1, tag1);
        ix++;
    }
}

void TagWidget::showTagsConfig()
{
    QDialog d(this);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
    auto *l = new QVBoxLayout;
    d.setLayout(l);
    QLabel lab(i18n("Configure Project Tags"), &d);
    QListWidget list(&d);
    l->addWidget(&lab);
    l->addWidget(&list);
    l->addWidget(buttonBox);
    for (DragButton *tb : qAsConst(tags)) {
        const QString color = tb->tag();
        const QString desc = tb->description();
        QIcon ic = tb->icon();
        auto *item = new QListWidgetItem(ic, desc, &list);
        item->setData(Qt::UserRole, color);
        item->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
    d.connect(buttonBox, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    d.connect(buttonBox, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    if (d.exec() != QDialog::Accepted) {
        return;
    }
    QMap <QString, QString> newTags;
    for (int i = 0; i < list.count(); i++) {
        QListWidgetItem *item = list.item(i);
        if (item) {
            newTags.insert(item->data(Qt::UserRole).toString(), item->text());
        }
    }
    rebuildTags(newTags);
    emit updateProjectTags(newTags);
}
