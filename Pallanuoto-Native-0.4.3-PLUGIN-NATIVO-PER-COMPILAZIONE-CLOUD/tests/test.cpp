#include "../src/plugin.cpp"
#include <QTemporaryDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <cassert>
#include <cstdlib>
#include <iostream>
static QString testPath;
extern "C" {
void bfree(void *p){free(p);}
void blog(int,const char*,...){}
char *obs_module_get_config_path(obs_module_t*,const char*){return strdup(testPath.toUtf8().constData());}
lookup_t *obs_module_load_locale(obs_module_t*,const char*,const char*){return nullptr;}
void text_lookup_destroy(lookup_t*){}
bool text_lookup_getstr(lookup_t*,const char*,const char**){return false;}
void *obs_frontend_get_main_window(void){return nullptr;}
void obs_frontend_add_event_callback(obs_frontend_event_cb,void*){}
void obs_frontend_remove_event_callback(obs_frontend_event_cb,void*){}
bool obs_frontend_add_dock_by_id(const char*,const char*,void*){return true;}
void obs_frontend_remove_dock(const char*){}
}
static QByteArray get(const QString &url,int expected=200){QNetworkAccessManager net;auto *reply=net.get(QNetworkRequest(QUrl(url)));QEventLoop wait;QObject::connect(reply,&QNetworkReply::finished,&wait,&QEventLoop::quit);QTimer::singleShot(5000,&wait,&QEventLoop::quit);wait.exec();assert(reply->isFinished());assert(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==expected);return reply->readAll();}
int main(int argc,char **argv){QApplication app(argc,argv);QTemporaryDir tmp;assert(tmp.isValid());testPath=tmp.path()+"/partita.json";QFile defaults(":/pallanuoto/default.json");assert(defaults.open(QIODevice::ReadOnly));auto s=QJsonDocument::fromJson(defaults.readAll()).object();assert(valid(s));
 auto fonts=s["typography"].toObject();assert(fonts.contains("lineups")&&fonts.contains("score")&&fonts.contains("intro"));
 auto invalidFont=s;auto altered=invalidFont["typography"].toObject();auto font=altered["lineups"].toObject();font["size"]=99;altered["lineups"]=font;invalidFont["typography"]=altered;assert(!valid(invalidFont));
 auto legacyCap=s;auto lg=legacyCap["home"].toObject();auto arr=lg["players"].toArray();auto first=arr[0].toObject();first.remove("captain");first["role"]="Capitano";arr[0]=first;auto second=arr[1].toObject();second.remove("captain");second["role"]="C";arr[1]=second;lg["players"]=arr;legacyCap["home"]=lg;
 auto converted=normalise(legacyCap)["home"].toObject()["players"].toArray();assert(converted[0].toObject()["captain"].toBool());assert(!converted[1].toObject()["captain"].toBool());assert(!converted[0].toObject().contains("role"));auto bad=s;bad["scene"]="bad";assert(!valid(bad));bad=s;bad["homeScore"]=-1;assert(!valid(bad));bad=s;auto home=bad["home"].toObject();home["logo"]="javascript:alert(1)";bad["home"]=home;assert(!valid(bad));

 auto legacy=s;legacy.remove("eventLogo");legacy["scene"]="lower";legacy["lowerName"]="vecchio";auto migrated=normalise(legacy);assert(valid(migrated));assert(migrated["scene"]=="hidden"&&!migrated.contains("lowerName"));
 const QString testLogo="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+a4y0AAAAASUVORK5CYII=";
 auto withLogo=s;withLogo["eventLogo"]=testLogo;assert(valid(withLogo));auto invalidLogo=s;invalidLogo["eventLogo"]="https://invalid.example/logo.png";assert(!valid(invalidLogo));QFile seed(testPath);assert(seed.open(QIODevice::WriteOnly));seed.write(QJsonDocument(withLogo).toJson());seed.close();
 {Control c(nullptr);auto *w=c.build();const auto base=QString("http://127.0.0.1:18789");assert(QJsonDocument::fromJson(get(base+"/api/state")).object()["scene"]=="hidden");assert(get(base+"/overlay.html").contains("stage"));assert(get(base+"/overlay-v043.html").contains("v=043"));assert(get(base+"/overlay-v042.html").contains("v=043"));assert(QJsonDocument::fromJson(get(base+"/api/version")).object()["version"]=="0.4.3");assert(get(base+"/style.css?v=043").contains("width:445px;height:113px"));assert(get(base+"/app.js").contains("both"));assert(get(base+"/app.js").contains("cap-red"));assert(get(base+"/app.js").contains("captain-tag"));get(base+"/server.js",404);
 auto *fontControl=w->findChild<QFontComboBox*>("fontFamily0");auto *sizeControl=w->findChild<QSpinBox*>("fontSize0");auto *boldControl=w->findChild<QCheckBox*>("fontBold0");auto *italicControl=w->findChild<QCheckBox*>("fontItalic0");assert(fontControl&&sizeControl&&boldControl&&italicControl);
 auto buttons=w->findChildren<QPushButton*>();for(auto *b:buttons)assert(!b->text().contains("Lower third",Qt::CaseInsensitive));bool found=false;for(auto *b:buttons)if(b->text()=="DUE FORMAZIONI COMPLETE"){b->click();found=true;}assert(found);assert(QJsonDocument::fromJson(get(base+"/api/state")).object()["scene"]=="both");
 sizeControl->setValue(33);boldControl->setChecked(true);italicControl->setChecked(true);
 for(auto *b:buttons)if(b->text()=="Pubblica dati")b->click();
 auto publishedFonts=QJsonDocument::fromJson(get(base+"/api/state")).object()["typography"].toObject()["lineups"].toObject();
 assert(publishedFonts["size"]==33&&publishedFonts["bold"]==true&&publishedFonts["italic"]==true);assert(QFileInfo::exists(testPath));assert(QJsonDocument::fromJson(get(base+"/api/state")).object()["eventLogo"]==testLogo);
 auto *hs=w->findChild<QSpinBox*>("liveHomeScore");auto *as=w->findChild<QSpinBox*>("liveAwayScore");auto *per=w->findChild<QLineEdit*>("livePeriod");assert(hs&&as&&per);
 for(auto *b:buttons)if(b->text()=="Risultato")b->click();
 for(auto *e:w->findChildren<QLineEdit*>())if(e->text()=="Rari Nantes")e->setText("NOME IN BOZZA");
 hs->setValue(7);as->setValue(5);per->selectAll();per->insert("2° TEMPO");
 auto live=QJsonDocument::fromJson(get(base+"/api/state")).object();assert(live["homeScore"]==7&&live["awayScore"]==5&&live["period"]=="2° TEMPO");assert(live["scene"]=="score");assert(live["home"].toObject()["name"]=="Rari Nantes");
 QFile stored(testPath);assert(stored.open(QIODevice::ReadOnly));assert(QJsonDocument::fromJson(stored.readAll()).object()["homeScore"]==7);
if(argc>1){if(QString::fromLocal8Bit(argv[1])=="--intro"){for(auto *e:w->findChildren<QLineEdit*>())if(e->text()=="NOME IN BOZZA")e->setText("Rari Nantes");for(auto *b:buttons)if(b->text()=="Presentazione")b->click();}else if(QString::fromLocal8Bit(argv[1])=="--both"){for(auto *e:w->findChildren<QLineEdit*>())if(e->text()=="NOME IN BOZZA")e->setText("Rari Nantes");for(auto *b:buttons)if(b->text()=="DUE FORMAZIONI COMPLETE")b->click();}std::cout<<"Anteprima server Qt: http://127.0.0.1:18789/overlay.html"<<std::endl;QTimer::singleShot(600000,&app,&QCoreApplication::quit);app.exec();delete w;return 0;}
 QTcpSocket stream;stream.connectToHost(QHostAddress::LocalHost,18789);assert(stream.waitForConnected(1000));stream.write("GET /events HTTP/1.1\r\nHost: 127.0.0.1:18789\r\n\r\n");QEventLoop loop;QObject::connect(&stream,&QTcpSocket::readyRead,&loop,&QEventLoop::quit);QTimer::singleShot(3000,&loop,&QEventLoop::quit);loop.exec();assert(stream.readAll().contains("data:"));hs->setValue(8);loop.exec();assert(stream.readAll().contains("\"homeScore\":8"));stream.disconnectFromHost();delete w;}
 {Control c(nullptr);auto *w=c.build();assert(QJsonDocument::fromJson(get("http://127.0.0.1:18789/api/state")).object()["scene"]=="hidden");delete w;}
 std::cout<<"PASS: migrazione capitano, formati font, calotte 1/13, live gol/periodo, isolamento bozze, persistenza, HTTP/SSE.\n";
}
