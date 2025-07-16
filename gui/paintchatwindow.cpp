#include "paintchatwindow.h"
#include "ui_paintchatwindow.h"

#include <QClipboard>
#include <QColorDialog>
#include <QBuffer>
#include <QPainter>
#include <QMessageBox>

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsidentity.h>
#include <unistd.h>
#include <util/imageutil.h>
#include <util/HandleRichText.h>

PaintChatWindow::PaintChatWindow(QWidget *parent, ChatId chatId, ChatWidget *chatWidget) :
	QMainWindow(parent), chatId(chatId), chatType(ChatWidget::CHATTYPE_UNKNOWN), chatWidget(chatWidget),
    ui(new Ui::PaintChatWindow)
{
    ui->setupUi(this);
    connect(ui->paintWidget,SIGNAL(haveUpdate()),SLOT(on_haveUpdate()));
    connect(ui->penWidthSpinBox,SIGNAL(valueChanged(int)),this,SLOT(penChanged()));

    ui->pushButton1px->setChecked(true);
    ui->pushButtonPen->setChecked(true);

    ui->paintWidget->color=Qt::black;
    ui->paintWidget->penWidth=1;

    timer=new QTimer(this);
    connect(timer,SIGNAL(timeout()),SLOT(on_timer()));
    timer->start(500);

    QIcon icon ;
    icon.addPixmap(QPixmap(":/images/colors.png"));
    setWindowIcon(icon);
    
    QPixmap pix(24, 24);
    pix.fill(currentColor);
    ui->pushButtonPrimaryColor->setIcon(pix);

    chatType = chatWidget->chatType();
// Disabled
//    if(chatType == ChatWidget::CHATTYPE_PRIVATE)
//    {
//        ui->label->setText(QString::fromStdString(std::string("Paintchat with ")+rsPeers->getPeerName(peerId)));
//        setWindowTitle(QString::fromStdString(rsPeers->getPeerName(peerId)+" - PaintChat"));

//        ImageResource res;
//        res.fromQImage(ui->paintWidget->getImage());
//        paintChatService->init(peerId,res);

//        ui->progressBar->hide();
//    }
    if(chatType == ChatWidget::CHATTYPE_LOBBY)
    {
        ui->progressBar->setRange(0,100);
        ui->progressBar->setValue(0);

        ChatLobbyId id;
		id = chatId.toLobbyId();
        std::string nick, lobby;

		RsGxsId gxsid;
		RsIdentityDetails details;
		rsMsgs->getIdentityForChatLobby(id, gxsid);
		for(int i=0;i<3;++i)
			if(rsIdentity->getIdDetails(gxsid,details))
				break ;
			else
				usleep(1000*300) ;
		nick = details.mNickname;

		ChatLobbyInfo info;
		rsMsgs->getChatLobbyInfo(id, info);
		lobby = info.lobby_name;

        std::string label = nick + "@" + lobby;
        ui->label->setText(QString::fromStdString(label));
        setWindowTitle(QString::fromStdString(label + " - PaintChat"));
    }

	ui->lblWarning->setVisible(false);
	MAX_LOBBY_MSG_SIZE = maxMessageSize();
}

PaintChatWindow::~PaintChatWindow()
{
    delete ui;
}

void PaintChatWindow::on_haveUpdate(){
    std::cerr<<"PaintChatWindow::on_haveUpdate()"<<std::endl;

    if(chatType == ChatWidget::CHATTYPE_PRIVATE)
    {
        updateImage();
    }
    if(chatType == ChatWidget::CHATTYPE_LOBBY)
    {
//        QImage i = ui->paintWidget->getImage();
//        QPainter p(&i);
//        p.setPen(Qt::black);
//        p.setBrush(Qt::NoBrush);
//        p.drawRect(0, 0, 302, 102);
//        ui->paintWidget->setImage(i);

		QImage img = ui->paintWidget->getImage();
		QString html;
		QImage opt;
		ImageUtil::optimizeSizeHtml(html, img, opt, -1, MAX_LOBBY_MSG_SIZE);
        ui->progressBar->setValue((html.size()*100)/MAX_LOBBY_MSG_SIZE);

		if(html.size()>MAX_LOBBY_MSG_SIZE)
		{
			ui->progressBar->setValue(100);
		}
		ui->lblImageSize->setText(QString("Image size: %1 bytes, maximum: %2 bytes").arg(html.size()).arg(MAX_LOBBY_MSG_SIZE));
		ui->lblWarning->setVisible(opt.width() < img.width());
    }
}

void PaintChatWindow::on_timer(){
//	if((chatType == ChatWidget::CHATTYPE_PRIVATE)&&(paintChatService->haveUpdate(chatId.toStdString()))){
//        std::cerr<<"PaintChatWindow::on_timer(): paintChatService->haveUpdate(peerId) returned true"<<std::endl;
//        updateImage();
//    }
}

void PaintChatWindow::updateImage(){
//	if(paintChatService->receivedInit(chatId.toStdString())){
//        ui->paintWidget->fillImage(Qt::white);
//        ImageResource res;
//        res.fromQImage(ui->paintWidget->getImage());
//		paintChatService->init(chatId.toStdString(),res);
//    }else{
//        ImageResource res;
//        res.fromQImage(ui->paintWidget->getImage());
//		ui->paintWidget->setImage(paintChatService->update(chatId.toStdString(),res).toQImage());
//    }
}

void PaintChatWindow::resetPenButtons(){
    ui->pushButton1px->setChecked(false);
    ui->pushButton4px->setChecked(false);
    ui->pushButton8px->setChecked(false);
}

void PaintChatWindow::on_pushButtonPrimaryColor_clicked()
{
  bool ok;
	QRgb color = QColorDialog::getRgba( Qt::black , &ok, window());
	if (ok) {
		ui->paintWidget->color=QColor(color);
		currentColor = QColor(color);
		colorChanged();
	}
}

void PaintChatWindow::colorChanged()
{
	QPixmap pix(24, 24);
	pix.fill(currentColor);
	ui->pushButtonPrimaryColor->setIcon(pix);
}

uint32_t PaintChatWindow::maxMessageSize()
{
	uint32_t maxMessageSize = 1;
	switch (chatWidget->chatType()) {
	case ChatWidget::CHATTYPE_UNKNOWN:
		break;
	case ChatWidget::CHATTYPE_PRIVATE:
		maxMessageSize = rsMsgs->getMaxMessageSecuritySize(RS_CHAT_TYPE_PRIVATE);
		break;
	case ChatWidget::CHATTYPE_LOBBY:
		maxMessageSize = rsMsgs->getMaxMessageSecuritySize(RS_CHAT_TYPE_LOBBY);
		break;
	case ChatWidget::CHATTYPE_DISTANT:
		maxMessageSize = rsMsgs->getMaxMessageSecuritySize(RS_CHAT_TYPE_DISTANT);
		break;
	}
	return maxMessageSize;
}

void PaintChatWindow::on_pushButtonPen_clicked()
{
    ui->pushButtonWhite->setChecked(false);
    ui->pushButtonPen->setChecked(true);

		ui->paintWidget->color= currentColor; 
}

void PaintChatWindow::on_pushButtonWhite_clicked()
{
    ui->pushButtonWhite->setChecked(true);
    ui->pushButtonPen->setChecked(false);
    ui->paintWidget->color=Qt::white;
}

void PaintChatWindow::on_pushButton1px_clicked()
{
    ui->paintWidget->penWidth=1;
	ui->penWidthSpinBox->setValue(1);
}

void PaintChatWindow::on_pushButton4px_clicked()
{
    ui->paintWidget->penWidth=4;
	ui->penWidthSpinBox->setValue(4);
}

void PaintChatWindow::on_pushButton8px_clicked()
{
    ui->paintWidget->penWidth=8;
	ui->penWidthSpinBox->setValue(8);
}

void PaintChatWindow::on_pushButtonClear_clicked()
{
    //käse tut ned, vermutlich wegen gleichen timestamps
    /*
    // not the best way to reset the image, because it causes the entire image to be transmitted
    // first overwrite with black
    ui->paintWidget->fillImage(Qt::black);
    updateImage();
    // then with white
    ui->paintWidget->fillImage(Qt::white);
    updateImage();
    */

    // andere Lösung:
    // sendet viele bytes, weil resource unkomprimiert ist
    // tut nicht, vielleicht zu große items?

    ui->paintWidget->fillImage(Qt::white);
//    ImageResource res;
//    res.fromQImage(ui->paintWidget->getImage());
//	paintChatService->init(chatId.toStdString(),res);
//	paintChatService->sendInit(chatId.toStdString(),res);

	on_haveUpdate();
}

void PaintChatWindow::on_pushButtonCopy_clicked()
{
    QApplication::clipboard()->setImage(ui->paintWidget->getImage());
}

//bool PaintChatWindow::imgToHtmlString(std::string& html, QImage img, int maxsize)
//{
//	QImage out;
//    QByteArray ba;
//    QBuffer buffer(&ba);
//	img = img.copy(0, 0, 300, 300);
//	int limit = (maxsize - std::string("<img src=\"data:image/png;base64,\"/>").size() - 2)*3/4;

//	//Cropping algoritmh from here: http://stackoverflow.com/questions/10678015/how-to-auto-crop-an-image-white-border-in-java
//	QRgb bc = img.pixel(0,0);
//	int l=img.width(), r=-1, t=img.height(), b=-1;
//	for(int y=0; y<img.height(); ++y) {
//		for(int x=0; x<img.width(); ++x) {
//			if(bc != img.pixel(x, y)) {
//				if(x<l) l=x;
//				if(x>r) r=x;
//				if(y<t) t=y;
//				if(y>b) b=y;
//			}
//		}
//	}
//	if(r == -1)
//		out = img.copy(0, 0, 1, 1); //TODO better handling of empty images
//	else
//		out = img.copy(l, t, r-l+1, b-t+1);

//    buffer.open(QIODevice::WriteOnly);
//	out.save(&buffer, "PNG", 0);
//	buffer.close();

//	if((ba.size() <= limit) || (maxsize == 0))
//	{
//		html = std::string("<img src=\"data:image/png;base64,") + ba.toBase64().data() + "\"/>";
//		return false;
//	}

//	//Downscale to fit into the limit
//	int scale = 1;
//	do{
//		scale *= 2;
//		out = img.scaledToWidth(img.width() / scale);
//		ba.clear();
//		buffer.open(QIODevice::WriteOnly);
//		out.save(&buffer, "PNG", 0);
//		buffer.close();
//	}while(ba.size() > limit);

//	html = std::string("<img src=\"data:image/png;base64,") + ba.toBase64().data() + "\"/>";
//	return true;
//}

void PaintChatWindow::on_pushButtonSend_clicked()
{
	QImage img = ui->paintWidget->getImage();
	QString html;
	QImage opt;
	if(ImageUtil::optimizeSizeHtml(html, img, opt, -1, MAX_LOBBY_MSG_SIZE)){
		rsMsgs->sendChat(ChatId(chatId), html.toUtf8().constData());
	}
}

void PaintChatWindow::penChanged()
{
    resetPenButtons();

    int width = ui->penWidthSpinBox->value();
    ui->paintWidget->penWidth= width;
}
