#include <QCoreApplication>
#include <iostream>
#include <QCommandLineParser>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStringList>
#include <QTextStream>
#include <QDirIterator>
#include "niv.h"

QStringList scanDir(QString dir) {
    QStringList files;
    QDirIterator it(dir , QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot , QDirIterator::Subdirectories);
    while (it.hasNext())
        files.append(it.next());
    return files;
}


QString createNivP(QHash<QString, QStringList> files, QString base, QString scriptFn, QString prefix, QString output) {


    NivBuilderFile result;
    result.name = output;
    qDebug() << "working on project " << output << "...";

    foreach (auto p, files.keys()) {
        qDebug() << "building partition " << p;
        QHash<QString, QByteArray> partition;
        foreach (auto s, files[p]) {
            BuilderItem item;
            QFile f(s);
            if (f.open(QFile::ReadOnly)) {
                item.data = f.readAll();
                f.close();
            } else
                qDebug() << "Error: cant open file " + s;
            item.filename = QString(s).replace(base, prefix);
            qDebug() << item.filename;
            partition[item.filename] = item.toByteArray();
        }
        result.partitions[p] = partition;
    }

    qDebug() << "building script...";

    QFile f(base + "/" + scriptFn);
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&f);
        result.script = in.readAll();
        f.close();
    } else
        qDebug() << "Error: cant open script bundle";

    qDebug() << "saving file ...";

    QFile out(base + "/" + output + ".nivp");
    if (out.open(QFile::WriteOnly)) {
        out.write(result.toByteArray());
        out.flush();
        out.close();
        return QString(base + "/" + output + ".nivp");
    } else
        qDebug() << "Error: cant save file " + base + "/" + output + ".nivp";
    return "";
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.addPositionalArgument("source", "Project file name");
    parser.process(a);
    if (parser.positionalArguments().length()) {
        QString name = parser.positionalArguments()[0];
        QFileInfo fi(name);
        QString baseDir = fi.absoluteDir().absolutePath();

        qDebug() << "NIV BUILDER v" + QString::number(NIV_VERSION);
        qDebug() << "parsing " << name;

        QFile f(name);
        if (f.open(QFile::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            ProjectConfig project = ProjectConfig::fromJson(doc.object());

            QHash<QString, QStringList> filelist;
            foreach (auto p, project.partitions) {
                filelist[p] = scanDir(baseDir + "/" + project.dataSrc + p);
            }

            QString fn = createNivP(filelist, baseDir, project.script, project.prefix, project.name);
            qDebug() << "done!";

            NivBuilderFile nivb = NivBuilderFile::fromFile(fn);
            NivStore store(fn, &nivb);
            store.loadPartition("images");

            a.quit();

        } else {
            qDebug() << "cant open project file";
            a.quit();
        }
    } else {
        qDebug() << "no file specified.";
    }

    return 0;
}
