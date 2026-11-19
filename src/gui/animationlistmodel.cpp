#include "animationlistmodel.h"
#include "kbanim.h"

AnimationListModel::AnimationListModel(QObject* parent) :
    QAbstractListModel(parent)
{
}

int AnimationListModel::rowCount(const QModelIndex& parent) const {
    if(parent.isValid())
        return 0;
    return _animations.count();
}

QVariant AnimationListModel::data(const QModelIndex& index, int role) const {
    if(!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return QVariant();
    KbAnim* anim = _animations.at(index.row());
    switch(role){
    case Qt::DisplayRole:
    case Qt::EditRole:
        return anim->name();
    case AnimationRole:
        return QVariant::fromValue<QObject*>(anim);
    case GuidRole:
        return anim->guid().toString();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> AnimationListModel::roleNames() const {
    QHash<int, QByteArray> names = QAbstractListModel::roleNames();
    names[AnimationRole] = "animation";
    names[GuidRole] = "guid";
    return names;
}

KbAnim* AnimationListModel::animation(int row) const {
    if(row < 0 || row >= _animations.count())
        return nullptr;
    return _animations.at(row);
}

void AnimationListModel::setAnimations(const QList<KbAnim*>& animations){
    beginResetModel();
    _animations = animations;
    endResetModel();
}

void AnimationListModel::append(KbAnim* animation){
    const int row = _animations.count();
    beginInsertRows(QModelIndex(), row, row);
    _animations.append(animation);
    endInsertRows();
}

void AnimationListModel::remove(KbAnim* animation){
    const int row = _animations.indexOf(animation);
    if(row < 0)
        return;
    beginRemoveRows(QModelIndex(), row, row);
    _animations.removeAt(row);
    endRemoveRows();
}

void AnimationListModel::renamed(KbAnim* animation){
    const int row = _animations.indexOf(animation);
    if(row < 0)
        return;
    const QModelIndex i = index(row);
    emit dataChanged(i, i, {Qt::DisplayRole, Qt::EditRole});
}
