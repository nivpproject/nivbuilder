#include <QJsonArray>
#include "niv.h"

Niv::Niv(QObject *parent)
    : QObject{parent}
{

}


QByteArray BuilderItem::toByteArray() const
{
    QByteArray result;
    QDataStream ds(&result, QDataStream::WriteOnly);
    ds.setVersion(QDataStream::Qt_6_2);
    ds << id << filename << type << text << data;
    return result;
}

BuilderItem BuilderItem::fromByteArray(QByteArray &ba)
{
    BuilderItem result;

    QDataStream ds(&ba, QDataStream::ReadOnly);
    ds.setVersion(QDataStream::Qt_6_2);

    ds >> result.id >> result.filename >> result.type >> result.text >> result.data;
    return result;
}

QByteArray NivBuilderFile::toByteArray() const
{
    QByteArray result;
    QDataStream ds(&result, QDataStream::WriteOnly);
    ds.setVersion(QDataStream::Qt_6_2);


    ds << MAGIC << int(NIV_VERSION) << partitionsPointer << name << compressString(script);

    QHash<QString, int> pointers;

    foreach(const QString &s, partitions.keys()) {
        pointers[s] = ds.device()->pos();
        Partition p;
        p.name = s;
        p.items = partitions[s];
        ds << p.toByteArray();
    }
    int pp = ds.device()->pos();

    ds << BOTTOM_HEADER_MAGIC << pointers;
    ds.device()->seek(8);
    ds << pp;

    return result;
}

NivBuilderFile NivBuilderFile::fromByteArray(QByteArray &ba)
{
    NivBuilderFile result;

    QDataStream ds(&ba, QDataStream::ReadOnly);
    ds.setVersion(QDataStream::Qt_6_2);
    ds >> result.magic;
    if (result.magic != MAGIC)
        return result;

    ds >> result.version >> result.partitionsPointer >> result.name;
    QByteArray scriptData;
    ds >> scriptData;
    result.script = uncompressString(scriptData);

    ds.device()->seek(result.partitionsPointer);

    int bhMagic = 0;
    ds >> bhMagic;
    if (bhMagic == BOTTOM_HEADER_MAGIC)
        ds >> result.pointers;

    return result;
}

NivBuilderFile NivBuilderFile::fromFile(QString filename)
{
    NivBuilderFile result;

    QFile f(filename);
    if (f.open(QFile::ReadOnly)) {
        QDataStream ds(&f);
        ds.setVersion(QDataStream::Qt_6_2);
        ds >> result.magic;
        if (result.magic != MAGIC)
            return result;

        ds >> result.version >> result.partitionsPointer >> result.name;
        QByteArray scriptData;
        ds >> scriptData;
        result.script = uncompressString(scriptData);

        ds.device()->seek(result.partitionsPointer);
        int bhMagic = 0;
        ds >> bhMagic;
        if (bhMagic == BOTTOM_HEADER_MAGIC)
            ds >> result.pointers;
        f.close();
    }
    return result;
}

void NivBuilderFile::loadPartition(QString name, QString filename)
{
    if (pointers.contains(name)) {
        QFile f(filename);
        if (f.open(QFile::ReadOnly)) {
            f.seek(pointers[name]);

            QDataStream ds(&f);
            ds.setVersion(QDataStream::Qt_6_2);
            QByteArray data;
            ds >> data;

            Partition p = Partition::fromByteArray(data);
            if (p.magic == Partition::MAGIC) {
                partitions[name] = p.items;
            }

            f.close();
        }
    }
}

void NivBuilderFile::loadAllPartitions(QString filename)
{
    foreach (const QString &s, pointers.keys()) {
        loadPartition(s, filename);
    }
}

void NivBuilderFile::unloadPartition(QString name)
{
    partitions.remove(name);
}

void NivBuilderFile::unloadAll()
{
    partitions.clear();
}

QByteArray NivBuilderFile::compressString(QString s)
{
    QByteArray out;
    QDataStream ds(&out, QDataStream::WriteOnly);
    ds << s;
    return qCompress(out, 9);
}

QString NivBuilderFile::uncompressString(QByteArray &ba)
{
    QByteArray data = qUncompress(ba);
    QDataStream ds(&data, QDataStream::ReadOnly);
    QString res;
    ds >> res;
    return res;
}

ProjectConfig ProjectConfig::fromJson(const QJsonObject &obj)
{
    ProjectConfig cfg;
    cfg.name = obj.value("name").toString();
    cfg.script = obj.value("script").toString();
    QJsonObject res = obj.value("data").toObject();
    cfg.dataSrc = res.value("src").toString();
    cfg.prefix = res.value("prefix").toString();

    QJsonArray pl = res.value("partitions").toArray();

    for (const auto &it : pl) {
        cfg.partitions.append(it.toString());
    }

    return cfg;
}

QByteArray Partition::toByteArray() const
{
    QByteArray result;
    QDataStream ds(&result, QDataStream::WriteOnly);
    ds.setVersion(QDataStream::Qt_6_2);

    QByteArray hashBytes;
    QDataStream hds(&hashBytes, QDataStream::WriteOnly);
    hds.setVersion(QDataStream::Qt_6_2);
    hds << items;

    ds << Partition::MAGIC << int(hashBytes.count()) << this->name;

    result.append(hashBytes);


    return result;
}

Partition Partition::fromByteArray(QByteArray &ba)
{
    Partition res;
    QDataStream ds(&ba, QDataStream::ReadOnly);
    ds.setVersion(QDataStream::Qt_6_2);
    ds >> res.magic;
    if (res.magic == Partition::MAGIC) {
        ds >> res.size >> res.name >> res.items;
    }
    return res;
}

Partition Partition::loadDescFromFile(QDataStream *ds)
{
    Partition res;
    (*ds) >> res.magic;
    if (res.magic == Partition::MAGIC) {
        (*ds) >> res.size >> res.name;
        ds->skipRawData(res.size);
    }
    return res;
}

NivStore::NivStore(QString filename, NivBuilderFile *src)
{
    this->filename = filename;
    this->src = src;
}

void NivStore::loadPartition(QString name)
{
    src->loadPartition(name, filename);
    auto p = src->partitions[name];
    foreach (const QString &s, p.keys()) {
        items[s] = BuilderItem::fromByteArray(p[s]);
    }
}

void NivStore::unloadPartition(QString name)
{
    if (src->partitions.contains(name)) {
        auto p = src->partitions[name];
        foreach (const QString &s, p.keys()) {
            items.remove(s);
        }
    }
}
