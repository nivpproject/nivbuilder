#ifndef NIV_H
#define NIV_H


#ifndef NIV_VERSION
#define NIV_VERSION 1
#endif

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QHash>
#include <QJsonObject>
#include <QStringList>
#include <QFile>


struct ProjectConfig {
    QString script, dataSrc, prefix, name;
    QStringList partitions;

    static ProjectConfig fromJson(const QJsonObject &obj);
};

struct BuilderItem {
    QByteArray toByteArray() const;
    static BuilderItem fromByteArray(QByteArray &ba);
    QString id;
    QString filename;
    QString type;
    QString text;
    QByteArray data;
};

struct Partition {
    QByteArray toByteArray() const;
    static Partition fromByteArray(QByteArray &ba);
    static Partition loadDescFromFile(QDataStream *ds);
    static const int MAGIC = 0x50415254;

    int magic;
    int size;
    QString name;

    QHash<QString, QByteArray> items;
};

struct NivBuilderFile {
    static const int MAGIC = 0x4e495650;
    static const int BOTTOM_HEADER_MAGIC = 0x4e495648;

    QByteArray toByteArray() const;
    static NivBuilderFile fromByteArray(QByteArray &ba);
    static NivBuilderFile fromFile(QString filename);

    void loadPartition(QString name, QString filename);
    void loadAllPartitions(QString filename);
    void unloadPartition(QString name);
    void unloadAll();

    static QByteArray compressString(QString s);
    static QString uncompressString(QByteArray &ba);


    int magic;
    int version;
    int partitionsPointer;
    QString name;
    QString script;

    QHash<QString, QHash<QString, QByteArray> > partitions;
    QHash<QString, int> pointers;

};


struct NivStore {

    NivStore() {src = 0;}
    NivStore(QString filename, NivBuilderFile *src);
    void loadPartition(QString name);
    void unloadPartition(QString name);

    QHash<QString, BuilderItem> items;
    QString filename;
    NivBuilderFile *src;

};



class Niv : public QObject
{
    Q_OBJECT
public:
    explicit Niv(QObject *parent = nullptr);

signals:

};

#endif // NIV_H
