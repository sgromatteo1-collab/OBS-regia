#if defined(__aarch64__) && defined(__APPLE__)
#include <arm_acle.h>
#endif
#include <functional>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <QApplication>
#include <QWidget>
#include <QPointer>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QColorDialog>
#include <QFontComboBox>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QSet>
#include <QRegularExpression>
#include <array>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("pallanuoto-native", "it-IT")
MODULE_EXPORT const char *obs_module_description(void) { return "Regia pallanuoto con grafica personalizzabile, calotte rosse e capitano"; }
MODULE_EXPORT const char *obs_module_name(void) { return "Pallanuoto Native"; }
MODULE_EXPORT const char *obs_module_author(void) { return "Pallanuoto Broadcast"; }

static const QStringList scenes={"hidden","both","home","away","score","intro"};
// Controlli di tipografia dedicati alle tre tipologie di grafica.
static QJsonObject typographyDefaults(){
 return {{"lineups",QJsonObject{{"family","Arial"},{"color","#ffffff"},{"size",25},{"bold",false},{"italic",false}}},
         {"score",QJsonObject{{"family","Arial"},{"color","#ffffff"},{"size",15},{"bold",true},{"italic",true}}},
         {"intro",QJsonObject{{"family","Arial"},{"color","#ffffff"},{"size",30},{"bold",true},{"italic",true}}}};
}
static QJsonObject normalise(QJsonObject s){
 if(s["scene"]=="lower")s["scene"]="hidden";
 if(!s.contains("eventLogo"))s["eventLogo"]="";
 for(const auto &k:{"lowerName","lowerRole","team","clock","page"})s.remove(k);
 auto fonts=s["typography"].toObject();const auto defaults=typographyDefaults();
 for(const auto &key:{"lineups","score","intro"}){
  auto f=fonts[key].toObject();auto def=defaults[key].toObject();
  for(auto it=def.begin();it!=def.end();++it)if(!f.contains(it.key()))f[it.key()]=it.value();
  fonts[key]=f;
 }
 s["typography"]=fonts;
 for(const auto &key:{"home","away"}){
  auto team=s[key].toObject();auto players=team["players"].toArray();QJsonArray migrated;
  bool captainFound=false;
  for(const auto &p:players){
   if(!p.isObject()){migrated.append(p);continue;}
   auto player=p.toObject();
   const auto oldRole=player["role"].toString().trimmed();
   const bool legacyCaptain=oldRole.compare("Capitano",Qt::CaseInsensitive)==0||oldRole.compare("C",Qt::CaseInsensitive)==0;
   const bool requested=player["captain"].isBool()?player["captain"].toBool():legacyCaptain;
   // Solo un capitano per squadra, anche importando archivi precedenti.
   const bool isCaptain=requested&&!captainFound;
   player.remove("role");
   player["captain"]=isCaptain;
   if(isCaptain)captainFound=true;
   migrated.append(player);
  }
  team["players"]=migrated;s[key]=team;
 }
 return s;
}
static bool validLogo(const QJsonValue &v){if(!v.isString())return false;const auto l=v.toString();return l.size()<=2000000&&(l.isEmpty()||QRegularExpression("^data:image/(png|jpeg|webp);base64,[A-Za-z0-9+/=]+$").match(l).hasMatch());}
static bool valid(const QJsonObject &s)
{
 if(!validLogo(s["eventLogo"]))return false;
 if(!scenes.contains(s["scene"].toString())) return false;
 const auto fonts=s["typography"].toObject();
 for(const auto &key:{"lineups","score","intro"}){
  const auto f=fonts[key].toObject();
  const auto family=f["family"].toString();
  if(!f["family"].isString()||family.size()>80||!QRegularExpression("^[\\p{L}\\p{N} ._+\\-]{1,80}$").match(family).hasMatch())return false;
  if(!f["color"].isString()||!QRegularExpression("^#[0-9a-fA-F]{6}$").match(f["color"].toString()).hasMatch())return false;
  if(!f["size"].isDouble()||f["size"].toDouble()!=f["size"].toInt()||f["size"].toInt()<10||f["size"].toInt()>60)return false;
  if(!f["bold"].isBool()||!f["italic"].isBool())return false;
 }
 for(const auto &k:{"title","subtitle","period"}) if(!s[k].isString()||s[k].toString().size()>160) return false;
 for(const auto &k:{"homeScore","awayScore"}) {auto v=s[k]; if(!v.isDouble()||v.toDouble()!=v.toInt()||v.toInt()<0||v.toInt()>999)return false;}
 for(const auto &k:{"home","away"}) {
  const auto t=s[k].toObject();
  for(const auto &key:{"name","short","staff","color","logo"})if(!t[key].isString())return false;
  if(t["name"].toString().size()>60||t["short"].toString().size()>8||t["staff"].toString().size()>600)return false;
  if(!QRegularExpression("^#[0-9a-fA-F]{6}$").match(t["color"].toString()).hasMatch())return false;
  const auto logo=t["logo"].toString();if(logo.size()>2000000||(!logo.isEmpty()&&!QRegularExpression("^data:image/(png|jpeg|webp);base64,[A-Za-z0-9+/=]+$").match(logo).hasMatch()))return false;
  if(!t["players"].isArray()||t["players"].toArray().size()>30)return false;
  int nCaptains=0;
  for(const auto &v:t["players"].toArray()){
   if(!v.isObject())return false;
   for(const auto &key:{"number","name"})if(!v.toObject()[key].isString()||v.toObject()[key].toString().size()>60)return false;
   if(!v.toObject()["captain"].isBool())return false;
   nCaptains+=v.toObject()["captain"].toBool()?1:0;
  }
  if(nCaptains>1)return false;
 }
 return true;
}
struct TeamUi { QLineEdit *name=nullptr,*shortName=nullptr; QPlainTextEdit *staff=nullptr; QTableWidget *players=nullptr; QPushButton *colorButton=nullptr; QString color,logo; };
struct StyleUi { QFontComboBox *family=nullptr; QPushButton *colorButton=nullptr; QSpinBox *size=nullptr; QCheckBox *bold=nullptr,*italic=nullptr; QString color; };
class Control final : public QObject {
 QTcpServer http;
 QSet<QTcpSocket*> streams;
 QJsonObject state;
 QString storage;
 QPointer<QWidget> panel;
 QLabel *status=nullptr;
 QLineEdit *title=nullptr,*subtitle=nullptr,*period=nullptr;
 QSpinBox *homeScore=nullptr,*awayScore=nullptr;
 QString eventLogo;
 QLabel *eventLogoStatus=nullptr;
 std::array<TeamUi,2> teams;
 std::array<StyleUi,3> styles;
 bool filling=false;
 const quint16 port=18789;
 QByteArray packet()const{return "data: "+QJsonDocument(state).toJson(QJsonDocument::Compact)+"\n\n";}
 void send(QTcpSocket *s,int code,const QByteArray &type,const QByteArray &body){s->write("HTTP/1.1 "+QByteArray::number(code)+(code==200?" OK\r\n":" Error\r\n")+"Content-Type: "+type+"\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n"+body);s->disconnectFromHost();}
 void readRequest(QTcpSocket *s){
  if(s->property("served").toBool())return;
  QByteArray b=s->property("request").toByteArray()+s->readAll();if(b.size()>16384){send(s,413,"text/plain","Request too large");return;}s->setProperty("request",b);
  if(!b.contains("\r\n\r\n"))return;s->setProperty("served",true);
  const auto lines=b.split('\n'); const auto first=lines[0].trimmed().split(' ');if(first.size()!=3||first[0]!="GET"){send(s,405,"text/plain","GET only");return;}
  QByteArray host;for(auto line:lines)if(line.toLower().startsWith("host:"))host=line.mid(5).trimmed();
  if(host!="127.0.0.1:18789"&&host!="localhost:18789"){send(s,403,"text/plain","Host non consentito");return;}
  const QByteArray route=first[1].split('?')[0];
  if(route=="/api/state"){send(s,200,"application/json; charset=utf-8",QJsonDocument(state).toJson(QJsonDocument::Compact));return;}
  if(route=="/events"){s->write("HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-cache\r\nConnection: keep-alive\r\n\r\n");streams.insert(s);s->write(packet());return;}
  if(route=="/api/version"){send(s,200,"application/json","{\"version\":\"0.4.3\",\"scoreWidth\":445,\"scoreHeight\":113}");return;}
  QString file;if(route=="/"||route=="/overlay.html"||route=="/overlay-v042.html"||route=="/overlay-v043.html")file="overlay.html";else if(route=="/style.css")file="style.css";else if(route=="/app.js")file="app.js";
  if(file.isEmpty()){send(s,404,"text/plain","Non trovato");return;}
  QFile f(":/pallanuoto/"+file);if(!f.open(QIODevice::ReadOnly)){send(s,500,"text/plain","Risorsa assente");return;}
  send(s,200,file.endsWith("css")?"text/css":file.endsWith("js")?"text/javascript":"text/html; charset=utf-8",f.readAll());
 }
 void fail(const QString &text){if(status)status->setText(text);}
 void collect(){
  state["title"]=title->text();state["subtitle"]=subtitle->text();state["period"]=period->text();state["homeScore"]=homeScore->value();state["awayScore"]=awayScore->value();state["eventLogo"]=eventLogo;
  auto fonts=state["typography"].toObject();
  const QStringList names={"lineups","score","intro"};
  for(int i=0;i<3;++i){const auto &u=styles[i];fonts[names[i]]=QJsonObject{{"family",u.family->currentFont().family()},{"color",u.color},{"size",u.size->value()},{"bold",u.bold->isChecked()},{"italic",u.italic->isChecked()}};}
  state["typography"]=fonts;
  for(int i=0;i<2;++i){auto &t=teams[i];auto obj=state[i==0?"home":"away"].toObject();obj["name"]=t.name->text();obj["short"]=t.shortName->text();obj["staff"]=t.staff->toPlainText();obj["color"]=t.color;obj["logo"]=t.logo;QJsonArray players;for(int row=0;row<t.players->rowCount();++row){QJsonObject a;a["number"]=t.players->item(row,0)?t.players->item(row,0)->text():"";a["name"]=t.players->item(row,1)?t.players->item(row,1)->text():"";a["captain"]=t.players->item(row,2)&&t.players->item(row,2)->checkState()==Qt::Checked;players.append(a);}obj["players"]=players;state[i==0?"home":"away"]=obj;}
 }
 bool persist(){QSaveFile f(storage);const auto bytes=QJsonDocument(state).toJson();return f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size()&&f.commit();}
 void publish(const QString &scene={}){
  const auto previous=state;collect();if(!scene.isEmpty())state["scene"]=scene;
  if(!valid(state)){state=previous;fail("Dati non validi: controlla lunghezza testi e distinte (max 30). Bozza non trasmessa.");return;}
  if(!persist()){state=previous;fail("Impossibile salvare la partita. Bozza non trasmessa.");return;}
  for(auto *s:streams)if(s->bytesToWrite()<5000000)s->write(packet());
  status->setText(http.isListening()?"PUBBLICATO · "+state["scene"].toString()+" · http://127.0.0.1:18789/overlay-v043.html":"Dati salvati, ma porta 18789 non disponibile: grafica non collegata.");
 }
 QLineEdit *edit(QFormLayout *form,const QString &label,int max=160){auto *e=new QLineEdit;e->setMaxLength(max);form->addRow(label,e);return e;}
 QPushButton *button(QBoxLayout *layout,const QString &label,const std::function<void()> &fn){auto *b=new QPushButton(label);layout->addWidget(b);connect(b,&QPushButton::clicked,this,fn);return b;}
 void liveValue(const QString &key,const QJsonValue &value){
  if(filling||state[key]==value)return;
  const auto old=state;state[key]=value;
  if(!valid(state)||!persist()){state=old;fail("Aggiornamento live non salvato. Controlla i dati e la cartella configurazione.");return;}
  const auto sockets=streams;for(auto *s:sockets){if(s->bytesToWrite()>5000000)s->abort();else s->write(packet());}
  status->setText(http.isListening()?"LIVE · Punteggio e periodo aggiornati automaticamente":"Dati salvati, ma server non disponibile sulla porta 18789");
 }
 static QTableWidgetItem *captainItem(bool captain){
  auto *item=new QTableWidgetItem(captain?"C":"");
  item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
  item->setCheckState(captain?Qt::Checked:Qt::Unchecked);
  item->setTextAlignment(Qt::AlignCenter);
  return item;
 }
 static void paintCap(QTableWidgetItem *item){
  if(!item)return;
  const auto n=item->text().trimmed();bool numberOk=false;const int number=n.toInt(&numberOk);const bool red=numberOk&&(number==1||number==13);
  item->setBackground(red?QBrush(QColor("#cf2035")):QBrush());
  item->setForeground(red?QBrush(Qt::white):QBrush());
  if(red)item->setToolTip("Calotta rossa obbligatoria");else item->setToolTip({});
 }
 void fill(){filling=true;
  title->setText(state["title"].toString());subtitle->setText(state["subtitle"].toString());period->setText(state["period"].toString());homeScore->setValue(state["homeScore"].toInt());awayScore->setValue(state["awayScore"].toInt());eventLogo=state["eventLogo"].toString();eventLogoStatus->setText(eventLogo.isEmpty()?"Nessun logo manifestazione":"Logo manifestazione caricato");
  const auto fonts=state["typography"].toObject();const QStringList names={"lineups","score","intro"};
  for(int i=0;i<3;++i){auto f=fonts[names[i]].toObject();auto &u=styles[i];u.family->setCurrentFont(QFont(f["family"].toString()));u.size->setValue(f["size"].toInt());u.bold->setChecked(f["bold"].toBool());u.italic->setChecked(f["italic"].toBool());u.color=f["color"].toString();u.colorButton->setText("Colore testo: "+u.color);u.colorButton->setStyleSheet("QPushButton{border-left:7px solid "+u.color+";}");}
  for(int i=0;i<2;++i){auto obj=state[i==0?"home":"away"].toObject();auto &t=teams[i];t.name->setText(obj["name"].toString());t.shortName->setText(obj["short"].toString());t.staff->setPlainText(obj["staff"].toString());t.color=obj["color"].toString();t.logo=obj["logo"].toString();t.colorButton->setText("Colore: "+t.color);auto players=obj["players"].toArray();t.players->setRowCount(players.size());for(int row=0;row<players.size();++row){auto a=players[row].toObject();t.players->setItem(row,0,new QTableWidgetItem(a["number"].toString()));t.players->setItem(row,1,new QTableWidgetItem(a["name"].toString()));t.players->setItem(row,2,captainItem(a["captain"].toBool()));paintCap(t.players->item(row,0));}}
 filling=false; }
 QWidget *teamTab(int i){
  auto *page=new QWidget;auto *v=new QVBoxLayout(page);auto *form=new QFormLayout;v->addLayout(form);auto &t=teams[i];t.name=edit(form,"Nome squadra",60);t.shortName=edit(form,"Sigla",8);
  auto *tools=new QHBoxLayout;v->addLayout(tools);t.colorButton=button(tools,"Colore",[this,i]{auto &u=teams[i];auto color=QColorDialog::getColor(QColor(u.color),panel,"Colore squadra");if(color.isValid()){u.color=color.name();u.colorButton->setText("Colore: "+u.color);}});
  button(tools,"Logo…",[this,i]{auto file=QFileDialog::getOpenFileName(panel,"Logo squadra",{},"Immagini (*.png *.jpg *.jpeg *.webp)");if(file.isEmpty())return;QFile f(file);if(f.size()>1000000||!f.open(QIODevice::ReadOnly)){fail("Logo non leggibile o oltre 1 MB");return;}auto ext=QFileInfo(file).suffix().toLower();if(ext=="jpg")ext="jpeg";teams[i].logo="data:image/"+ext+";base64,"+QString::fromLatin1(f.readAll().toBase64());fail("Logo caricato in bozza: premi Pubblica dati");});
  button(tools,"Rimuovi logo",[this,i]{teams[i].logo.clear();fail("Logo rimosso in bozza: premi Pubblica dati");});
  v->addWidget(new QLabel("Calotta · Nome · C (capitano). Una sola C per squadra; calotte 1 e 13 sempre rosse."));t.players=new QTableWidget(0,3);t.players->setHorizontalHeaderLabels({"Calotta","Nome e cognome","Capitano (C)"});t.players->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);t.players->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);t.players->setMinimumHeight(280);v->addWidget(t.players);
  connect(t.players,&QTableWidget::itemChanged,this,[this,i](QTableWidgetItem *item){
   if(filling||!item)return;auto *table=teams[i].players;
   QSignalBlocker block(table); // evita segnali ricorsivi durante l'esclusione
   if(item->column()==0)paintCap(item);
   if(item->column()==2){const bool chosen=item->checkState()==Qt::Checked;item->setText(chosen?"C":"");if(chosen)for(int r=0;r<table->rowCount();++r)if(r!=item->row())if(auto *other=table->item(r,2)){other->setCheckState(Qt::Unchecked);other->setText("");}}
  });
  auto *rowTools=new QHBoxLayout;v->addLayout(rowTools);button(rowTools,"+ Atleta",[this,i]{auto *table=teams[i].players;if(table->rowCount()>=30){fail("Massimo 30 atleti");return;}int row=table->rowCount();table->insertRow(row);table->setItem(row,0,new QTableWidgetItem(""));table->setItem(row,1,new QTableWidgetItem(""));table->setItem(row,2,captainItem(false));});button(rowTools,"Rimuovi selezionato",[this,i]{auto *table=teams[i].players;if(table->currentRow()>=0)table->removeRow(table->currentRow());});
  button(rowTools,"Importa distinta…",[this,i]{auto file=QFileDialog::getOpenFileName(panel,"Importa distinta numero;nome;C",{},"Testo UTF-8 (*.txt *.csv)");if(file.isEmpty())return;QFile f(file);if(f.size()>100000||!f.open(QIODevice::ReadOnly)){fail("File non leggibile");return;}const auto lines=QString::fromUtf8(f.readAll()).split('\n',Qt::SkipEmptyParts);if(lines.size()>30){fail("Massimo 30 righe");return;}if(QMessageBox::question(panel,"Sostituire distinta?","La distinta in bozza verrà sostituita dal file.")!=QMessageBox::Yes)return;auto *table=teams[i].players;table->setRowCount(lines.size());bool captainSet=false;for(int r=0;r<lines.size();++r){const auto fields=lines[r].trimmed().split(';');table->setItem(r,0,new QTableWidgetItem(fields.value(0).trimmed()));table->setItem(r,1,new QTableWidgetItem(fields.value(1).trimmed()));const auto cap=fields.value(2).trimmed();const bool captain=!captainSet&&(cap.compare("C",Qt::CaseInsensitive)==0||cap.compare("Capitano",Qt::CaseInsensitive)==0);if(captain)captainSet=true;table->setItem(r,2,captainItem(captain));paintCap(table->item(r,0));}});
  v->addWidget(new QLabel("Staff — massimo 600 caratteri; consigliate 2–3 righe brevi"));t.staff=new QPlainTextEdit;t.staff->setMaximumHeight(100);v->addWidget(t.staff);return page;
 }
QWidget *styleTab(){
 auto *page=new QWidget;auto *outer=new QVBoxLayout(page);
 outer->addWidget(new QLabel("Personalizza i testi visibili in OBS. Premi 'Pubblica dati' in Regia per applicare le modifiche."));
 auto *tabs=new QTabWidget;outer->addWidget(tabs);
 const QStringList sceneNames={"Formazioni","Risultato","Presentazione"};
 const int suggested[]={25,15,30};
 for(int i=0;i<3;++i){
  auto *scenePage=new QWidget;auto *v=new QVBoxLayout(scenePage);auto *form=new QFormLayout;v->addLayout(form);
  auto &u=styles[i];u.family=new QFontComboBox;u.family->setObjectName("fontFamily"+QString::number(i));form->addRow("Font installato",u.family);
  u.size=new QSpinBox;u.size->setRange(10,60);u.size->setSuffix(" px");u.size->setValue(suggested[i]);u.size->setObjectName("fontSize"+QString::number(i));form->addRow("Altezza testo",u.size);
  u.bold=new QCheckBox("Grassetto");u.bold->setObjectName("fontBold"+QString::number(i));form->addRow("Spessore",u.bold);
  u.italic=new QCheckBox("Corsivo");u.italic->setObjectName("fontItalic"+QString::number(i));form->addRow("Stile",u.italic);
  u.colorButton=new QPushButton("Colore testo");u.colorButton->setObjectName("fontColor"+QString::number(i));form->addRow("Colore",u.colorButton);
  connect(u.colorButton,&QPushButton::clicked,this,[this,i]{auto &style=styles[i];auto result=QColorDialog::getColor(QColor(style.color),panel,"Seleziona colore testo");if(result.isValid()){style.color=result.name();style.colorButton->setText("Colore testo: "+style.color);}});
  v->addWidget(new QLabel(i==0?"Il numero 1 e 13 resta ROSSO anche cambiando il colore del testo.":i==1?"Il risultato conserva il riquadro 445 × 113 px.":"Titolo, squadre e sottotitolo seguono la tipografia impostata."));v->addStretch();tabs->addTab(scenePage,sceneNames[i]);
 }
 return page;
}
public:
 explicit Control(QObject *parent):QObject(parent){
  char *config=obs_module_config_path("partita.json");storage=QString::fromUtf8(config);bfree(config);QDir().mkpath(QFileInfo(storage).absolutePath());QFile defaults(":/pallanuoto/default.json");defaults.open(QIODevice::ReadOnly);state=normalise(QJsonDocument::fromJson(defaults.readAll()).object());QFile saved(storage);if(saved.open(QIODevice::ReadOnly)){auto candidate=normalise(QJsonDocument::fromJson(saved.readAll()).object());if(valid(candidate))state=candidate;}
  state["scene"]="hidden"; // Avvio sempre fuori onda, dati conservati.
  connect(&http,&QTcpServer::newConnection,this,[this]{while(http.hasPendingConnections()){auto *s=http.nextPendingConnection();connect(s,&QTcpSocket::readyRead,this,[this,s]{readRequest(s);});connect(s,&QTcpSocket::disconnected,this,[this,s]{streams.remove(s);s->deleteLater();});QTimer::singleShot(10000,s,[s]{if(!s->property("served").toBool())s->disconnectFromHost();});}});
  http.listen(QHostAddress::LocalHost,port);auto *heartbeat=new QTimer(this);heartbeat->setInterval(15000);connect(heartbeat,&QTimer::timeout,this,[this]{for(auto *s:streams)if(s->bytesToWrite()<5000000)s->write(": heartbeat\n\n");});heartbeat->start();
 }
 ~Control()override{http.close();const auto sockets=streams;streams.clear();for(auto *s:sockets){QObject::disconnect(s,nullptr,this,nullptr);s->abort();s->deleteLater();}}
 QWidget *build(){
  panel=new QWidget;auto *outer=new QVBoxLayout(panel);outer->addWidget(new QLabel("Pallanuoto Native 0.4.3 · Risultato 445 × 113 px"));status=new QLabel;status->setWordWrap(true);status->setText(http.isListening()?"Pronto · Fuori onda · Server integrato: 18789":"ERRORE: porta 18789 occupata. Chiudi altre istanze e riavvia OBS.");outer->addWidget(status);
  auto *tabs=new QTabWidget;outer->addWidget(tabs);auto *regia=new QWidget;auto *v=new QVBoxLayout(regia);
  auto *controls=new QGridLayout;v->addLayout(controls);const QStringList names={"Nascondi tutto","DUE FORMAZIONI COMPLETE","Solo casa","Solo ospiti","Risultato","Presentazione"};for(int i=0;i<scenes.size();++i){auto *b=new QPushButton(names[i]);controls->addWidget(b,i/2,i%2);connect(b,&QPushButton::clicked,this,[this,i]{publish(scenes[i]);});}
  auto *form=new QFormLayout;v->addLayout(form);title=edit(form,"Evento");subtitle=edit(form,"Sottotitolo");period=edit(form,"Periodo · LIVE");homeScore=new QSpinBox;awayScore=new QSpinBox;homeScore->setRange(0,999);awayScore->setRange(0,999);homeScore->setObjectName("liveHomeScore");awayScore->setObjectName("liveAwayScore");period->setObjectName("livePeriod");form->addRow("Gol casa · LIVE",homeScore);form->addRow("Gol ospiti · LIVE",awayScore);
  auto *eventTools=new QHBoxLayout;v->addLayout(eventTools);eventLogoStatus=new QLabel;v->addWidget(eventLogoStatus);
  button(eventTools,"Logo manifestazione…",[this]{auto file=QFileDialog::getOpenFileName(panel,"Logo manifestazione",{},"Immagini (*.png *.jpg *.jpeg *.webp)");if(file.isEmpty())return;QFile f(file);if(f.size()>1000000||!f.open(QIODevice::ReadOnly)){fail("Logo non leggibile o oltre 1 MB");return;}auto ext=QFileInfo(file).suffix().toLower();if(ext=="jpg")ext="jpeg";eventLogo="data:image/"+ext+";base64,"+QString::fromLatin1(f.readAll().toBase64());eventLogoStatus->setText("Logo manifestazione in bozza · premi Pubblica o Presentazione");});
  button(eventTools,"Rimuovi logo manifestazione",[this]{eventLogo.clear();eventLogoStatus->setText("Logo rimosso in bozza · premi Pubblica o Presentazione");});
  connect(homeScore,qOverload<int>(&QSpinBox::valueChanged),this,[this](int value){liveValue("homeScore",value);});
  connect(awayScore,qOverload<int>(&QSpinBox::valueChanged),this,[this](int value){liveValue("awayScore",value);});
  connect(period,&QLineEdit::textEdited,this,[this](const QString &value){liveValue("period",value);});
  v->addWidget(new QLabel("Punteggio e periodo sono LIVE. Gli altri dati sono in bozza fino a Pubblica."));
  button(v,"Pubblica dati",[this]{publish();});button(v,"Apri anteprima overlay",[this]{QDesktopServices::openUrl(QUrl("http://127.0.0.1:18789/overlay-v043.html"));});
  auto *archive=new QHBoxLayout;v->addLayout(archive);button(archive,"Esporta partita…",[this]{const auto old=state;collect();if(!valid(state)){state=old;fail("Bozza non valida: esportazione annullata");return;}const auto draft=state;state=old;auto name=QFileDialog::getSaveFileName(panel,"Esporta bozza", "partita.json","JSON (*.json)");if(name.isEmpty())return;QSaveFile file(name);auto bytes=QJsonDocument(draft).toJson();if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit())fail("Esportazione fallita");});
  button(archive,"Importa partita…",[this]{auto name=QFileDialog::getOpenFileName(panel,"Importa partita",{},"JSON (*.json)");if(name.isEmpty())return;QFile f(name);if(f.size()>5000000||!f.open(QIODevice::ReadOnly)){fail("File non leggibile o troppo grande");return;}auto next=normalise(QJsonDocument::fromJson(f.readAll()).object());if(!valid(next)){fail("Formato partita non valido");return;}if(QMessageBox::question(panel,"Importare partita?","Sostituire i dati correnti e andare fuori onda?")!=QMessageBox::Yes)return;const auto old=state;next["scene"]="hidden";state=next;if(!persist()){state=old;fail("Importazione non salvata");return;}fill();for(auto *s:streams)s->write(packet());fail("Partita importata · Fuori onda");});
  v->addWidget(new QLabel("OBS → Sorgente Browser → URL:\nhttp://127.0.0.1:18789/overlay-v043.html\n1920 × 1080 · Sfondo trasparente\nLe modifiche vanno in onda con Pubblica o un pulsante scena."));v->addStretch();
  auto *scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setWidget(regia);tabs->addTab(scroll,"Regia");tabs->addTab(teamTab(0),"Casa");tabs->addTab(teamTab(1),"Ospiti");tabs->addTab(styleTab(),"Grafica e font");fill();return panel;
 }
};
static QPointer<Control> controller;
static bool dockAdded=false;
static void setup(enum obs_frontend_event event,void*){
 if(event!=OBS_FRONTEND_EVENT_FINISHED_LOADING||controller)return;
 controller=new Control(static_cast<QWidget*>(obs_frontend_get_main_window()));
 QWidget *widget=controller->build();dockAdded=obs_frontend_add_dock_by_id("pallanuoto-native-dock","Pallanuoto · Regia 0.4.3",widget);
 if(!dockAdded){delete widget;delete controller;blog(LOG_ERROR,"[pallanuoto-native] Impossibile aggiungere il pannello");}
}
bool obs_module_load(void){obs_frontend_add_event_callback(setup,nullptr);blog(LOG_INFO,"[pallanuoto-native] Caricato 0.4.3");return true;}
void obs_module_unload(void){obs_frontend_remove_event_callback(setup,nullptr);if(dockAdded){obs_frontend_remove_dock("pallanuoto-native-dock");dockAdded=false;}delete controller;}
