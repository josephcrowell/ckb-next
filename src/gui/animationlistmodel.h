#ifndef ANIMATIONLISTMODEL_H
#define ANIMATIONLISTMODEL_H

#include <QAbstractListModel>
#include <QList>

class KbAnim;

// The animations of one lighting mode, in KbLight::animList() order.
// LightController keeps it in step with the list; it doesn't watch KbLight itself.
class AnimationListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        AnimationRole = Qt::UserRole + 1,   // KbAnim* as QObject*
        GuidRole,                           // QString
    };

    explicit AnimationListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    KbAnim* animation(int row) const;

    void setAnimations(const QList<KbAnim*>& animations);
    void append(KbAnim* animation);
    void remove(KbAnim* animation);
    void renamed(KbAnim* animation);

private:
    QList<KbAnim*> _animations;
};

#endif // ANIMATIONLISTMODEL_H
