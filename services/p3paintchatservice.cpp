#include "p3paintchatservice.h"

#include "services/paintchatitems.h"
#include <iostream>

PaintChatService *paintChatService=NULL;

p3PaintChatService::p3PaintChatService(RsPluginHandler *handler):
	RsPQIService(RS_SERVICE_TYPE_PAINTCHAT_PLUGIN,0,handler)//---------frage: was ist configtype?????
{
    addSerialType(new RsPaintChatSerialiser());
}

p3PaintChatService::~p3PaintChatService(){
    std::cerr<<"p3PaintChatService::~p3PaintChatService() deleting connections&engines..."<<std::endl;
    {
    std::map<std::string,Connection*>::iterator it;
    for(it=connections.begin();it!=connections.end();it++){
        std::cerr<<"p3PaintChatService::~p3PaintChatService() deleting connection "<<it->first<<std::endl;
        delete it->second;
    }
    }
    {
    std::map<std::string,SyncEngine<ImageResource,ImageDiff>*>::iterator it;
    for(it=syncEngines.begin();it!=syncEngines.end();it++){
        std::cerr<<"p3PaintChatService::~p3PaintChatService() deleting engine "<<it->first<<std::endl;
        delete it->second;
    }
    }
}

int p3PaintChatService::tick(){
    //std::cerr<<"p3PaintChatService::tick()"<<std::endl;
    // process incoming
    RsItem *item = NULL;
    while(NULL != (item = recvItem()))
    {
        std::cerr<<"p3PaintChatService::tick(): received Item from "<<item->PeerId()<<std::endl;
        RsPaintChatItem *paintChatItem=dynamic_cast<RsPaintChatItem*>(item);
		std::map<std::string,SyncEngine<ImageResource,ImageDiff>*>::iterator it=syncEngines.find(item->PeerId().toStdString());
        SyncEngine<ImageResource,ImageDiff>* engine;
        if(it==syncEngines.end()){
            std::cerr<<"p3PaintChatService::tick(): no SyncEngine found for "<<item->PeerId()<<std::endl;
			engine=addPeer(item->PeerId().toStdString());
        }else{
            engine=it->second;
        }
        if(paintChatItem->command==COMMAND_INIT){
            std::cerr<<"p3PaintChatService::tick(): COMMAND_INIT"<<item->PeerId()<<std::endl;
			receivedInits.push_back(item->PeerId().toStdString());
            /*
            ImageResource res;
            res.deserialise(paintChatItem->binData.bin_data,paintChatItem->binData.bin_len);
            engine->init(res);
            */
        }
        if(paintChatItem->command==COMMAND_SYNC){
            std::cerr<<"p3PaintChatService::tick(): COMMAND_SYNC"<<item->PeerId()<<std::endl;
            engine->processData(paintChatItem->binData.bin_data,paintChatItem->binData.bin_len);
        }
        delete item;
    }
    return 0;
}

void p3PaintChatService::init(std::string id, ImageResource res){
    SyncEngine<ImageResource,ImageDiff>* engine=addPeer(id);
    engine->init(res);
}

void p3PaintChatService::sendInit(std::string id, ImageResource res){
    std::cerr<<"p3PaintChatService::PaintChatConnection::sendInit()"<<std::endl;
    RsPaintChatItem* item=new RsPaintChatItem();
    item->command=COMMAND_INIT;

    // tut nicht, eventuell bild zu gro�?
    /*
    uint32_t size=res.serial_size();
    void* buf=malloc(size);
    res.serialise(buf,size);
    item->binData.setBinData(buf,size);
    free(buf);
    */

	item->PeerId(RsPeerId(id));
    // der service wird eigent�mer des items
    sendItem(item);
}

bool p3PaintChatService::haveUpdate(std::string id){
    std::list<std::string>::iterator initIt=receivedInits.begin();
    bool receivedInit=false;
    for(;initIt!=receivedInits.end();initIt++){
        if(*initIt==id){
            receivedInit=true;
        }
    }
    // k�nnte eigentlich auch ohne syncEngine gehen
    std::map<std::string,SyncEngine<ImageResource,ImageDiff>*>::iterator it=syncEngines.find(id);
    if(it!=syncEngines.end()){
        return (it->second->haveUpdate() || receivedInit);
    }else{
        return false;
    }
}

bool p3PaintChatService::receivedInit(std::string id){
    uint32_t sizeBefore=receivedInits.size();
    receivedInits.remove(id);
    if(receivedInits.size()!=sizeBefore){
        return true;
    }else{
        return false;
    }
}

ImageResource p3PaintChatService::update(std::string id, ImageResource res){
    std::map<std::string,SyncEngine<ImageResource,ImageDiff>*>::iterator it=syncEngines.find(id);
    if(it!=syncEngines.end()){
        return it->second->update(res);
    }else{
        std::cerr<<"p3PaintChatService::update(): no SyncEngine found for "<<id<<std::endl;
        SyncEngine<ImageResource,ImageDiff>* engine=addPeer(id);
        return engine->update(res);
	}
}

RsServiceInfo p3PaintChatService::getServiceInfo()
{
	const std::string L4R_APP_NAME = "PaintChat";
	const uint16_t L4R_APP_MAJOR_VERSION  =       1;
	const uint16_t L4R_APP_MINOR_VERSION  =       0;
	const uint16_t L4R_MIN_MAJOR_VERSION  =       1;
	const uint16_t L4R_MIN_MINOR_VERSION  =       0;

	return RsServiceInfo(RS_SERVICE_TYPE_PAINTCHAT_PLUGIN,
						 L4R_APP_NAME,
						 L4R_APP_MAJOR_VERSION,
						 L4R_APP_MINOR_VERSION,
						 L4R_MIN_MAJOR_VERSION,
						 L4R_MIN_MINOR_VERSION);
}

bool p3PaintChatService::saveList(bool &cleanup, std::list<RsItem *> &)
{
	cleanup = true;
	return true;
}

bool p3PaintChatService::loadList(std::list<RsItem *> &load)
{
	load.clear();
	return true;
}

SyncEngine<ImageResource,ImageDiff>* p3PaintChatService::addPeer(std::string peerId){
    std::cerr<<"p3PaintChatService::addPeer(\""<<peerId<<"\")"<<std::endl;
    PaintChatConnection* conn=new PaintChatConnection(peerId,this);
    SyncEngine<ImageResource,ImageDiff>* engine=new SyncEngine<ImageResource,ImageDiff>(conn);
    connections[peerId]=conn;
    syncEngines[peerId]=engine;
    return engine;
}

p3PaintChatService::PaintChatConnection::PaintChatConnection(std::string connId, p3PaintChatService *service):
    connId(connId),
    service(service){
}

void p3PaintChatService::PaintChatConnection::sendData(void *data, const uint32_t &size){
    std::cerr<<"p3PaintChatService::PaintChatConnection::sendData(): sending "<<size<<" bytes of data to "<<connId<<std::endl;
    RsPaintChatItem* item=new RsPaintChatItem();
    item->command=p3PaintChatService::COMMAND_SYNC;
    item->binData.setBinData(data,size);
	item->PeerId(RsPeerId(connId));
    item->print(std::cerr,1);
    // der service wird eigent�mer des items
    service->sendItem(item);
}
