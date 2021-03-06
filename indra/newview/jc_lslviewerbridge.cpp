/* Copyright (c) 2009
 *
 * Modular Systems All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS �AS IS�
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "llviewerprecompiledheaders.h"

#include "jc_lslviewerbridge.h"
#include "llchatbar.h"
#include "llagent.h"
#include "stdtypes.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "lluuid.h"
#include "llfilepicker.h"
#include "llassetstorage.h"
#include "llviewerobjectlist.h"

#include "importtracker.h"

#include "llviewerobject.h"

#include "llvoavatar.h"

#include "llinventorymodel.h"

#include "lltooldraganddrop.h"

#include "llmd5.h"

#include "llstartup.h"

#include "llcontrol.h"

#include "llviewercontrol.h"

#include "llviewergenericmessage.h"



#define vCatType (LLAssetType::EType)128
#define vBridgeName "#LSL<->Client Bridge v0.04"
#define vBridgeOpCat "#Meta7"

void cmdline_printchat(std::string message);

//static
U8 JCLSLBridge::sBridgeStatus;
//static
JCLSLBridge* JCLSLBridge::sInstance;

LLViewerObject* JCLSLBridge::sBridgeObject;

U32 JCLSLBridge::lastcall;

std::map<U32,JCBridgeCallback*> JCLSLBridge::callback_map;

S32 JCLSLBridge::l2c;
bool JCLSLBridge::l2c_inuse;

JCLSLBridge::JCLSLBridge() : LLEventTimer( (F32)1.0 )
{
	if(sInstance)
	{
		////cmdline_printchat("duplicate bridge?");
		delete this;
	}else
	{
		////cmdline_printchat("instanciated bridge");
		sInstance = this;
		lastcall = 0;
		l2c = 0;
		l2c_inuse = false;
		//getPermissions();
	}
}
JCLSLBridge::~JCLSLBridge()
{
}

bool JCLSLBridge::lsltobridge(std::string message, std::string from_name, LLUUID source_id, LLUUID owner_id)
{
	if(message == "someshit")
	{
		////cmdline_printchat("got someshit from "+source_id.asString());
		
		return true;
	}else
	{
		std::string clip = message.substr(0,5);
		if(clip == "#@#@#")
		{
			std::string rest = message.substr(5);
			LLSD arguments = JCLSLBridge::parse_string_to_list(rest, '|');
			////cmdline_printchat(std::string(LLSD::dumpXML(arguments)));
			U32 call = atoi(arguments[0].asString().c_str());
			if(call)
			{
				arguments.erase(0);
				////cmdline_printchat(std::string(LLSD::dumpXML(arguments)));
				callback_fire(call, arguments);
				return true;
			}
		}else if(clip == "rotat")
		{
			LLViewerObject* obj = gObjectList.findObject(source_id);
			if(obj)
			{
				if(obj->isAttachment())
				{
					if(owner_id == gAgent.getID())
					{
						std::string rot = message.substr(5);
						LLQuaternion rotation;
						if(LLQuaternion::parseQuat(rot,&rotation))
						{
							//cmdline_printchat("script rotating!");
							gAgent.getAvatarObject()->setRotation(rotation,TRUE);
						}else
						{
							//cmdline_printchat("script rotate failed");
						}
					}
				}
			}
		}else if(message.substr(0,3) == "l2c")
		{
			std::string lolnum = message.substr(3);
			//cmdline_printchat("num="+lolnum);
			l2c = atoi(lolnum.c_str());
			//cmdline_printchat("rnum="+llformat("%d",l2c));
			l2c_inuse = true;
			return true;
		}
	}
	return false;
}
void send_chat_from_viewer(std::string utf8_out_text, EChatType type, S32 channel);
void JCLSLBridge::bridgetolsl(std::string cmd, JCBridgeCallback* cb)
{
	if(sBridgeStatus == ACTIVE)
	{
		std::string chat = llformat("%d",registerCB(cb)) + "|"+cmd;
		send_chat_from_viewer(chat, CHAT_TYPE_WHISPER, l2c_inuse ? l2c : JCLSLBridge::bridge_channel(gAgent.getID()));
	}else
	{
		////cmdline_printchat("bridge not RECHAN");
		delete cb;
	}
}

std::string md5hash(const std::string &text, U32 thing)
{
	char temp[MD5HEX_STR_SIZE];
	LLMD5 toast((const U8*)text.c_str(), thing);
	toast.hex_digest(temp);
	return std::string(temp);
}

/*LLUUID md5hash(const std::string &text, U32 thing)
{
 LLUUID temp;
 LLMD5 toast((const U8*)text.c_str(), thing);
 toast.raw_digest(temp.mData);
 return temp;
}*/

S32 JCLSLBridge::bridge_channel(LLUUID user)
{
	if(l2c == 0 || l2c_inuse == false)
	{
		std::string tmps = md5hash(user.asString(),1);
		int i = (int)strtol((tmps.substr(0, 7) + "\n").c_str(),(char **)NULL,16);
		return (S32)i;
	}else return l2c;
}

class ObjectBNameMatches : public LLInventoryCollectFunctor
{
public:
	ObjectBNameMatches(std::string name)
	{
		sName = name;
	}
	virtual ~ObjectBNameMatches() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if(item)
		{
			//LLViewerInventoryCategory* folderp = gInventory.getCategory((item->getParentUUID());
			return (item->getName() == sName);// && cat->getName() == "#v");
		}
		return false;
	}
private:
	std::string sName;
};



const LLUUID& JCLSLBridge::findInventoryByName(const std::string& object_name)
{
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	ObjectBNameMatches objectnamematches(object_name);
	gInventory.collectDescendentsIf(gAgent.getInventoryRootID(),cats,items,FALSE,objectnamematches);

	if (items.count())
	{
		return items[0]->getUUID();
	}
	return LLUUID::null;
}

bool isworn(LLUUID item)
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if(avatar && avatar->isWearingAttachment(item) )
	{
		return true;
	}
	return false;
}

BOOL JCLSLBridge::tick()
{
	if(LLStartUp::getStartupState() >= STATE_INVENTORY_SEND)
	{
		switch(sBridgeStatus)
		{
		case UNINITIALIZED:
			{
				/*
				We're going to just completely disable this in code, it's shit - CW
				
				if(!gSavedSettings.getBOOL("Meta7BuildBridge"))
				{
				*/
					sBridgeStatus = FAILED;
					break;
				//}

				//cmdline_printchat("initializing");//<< llendl;
				LLUUID item_id = findInventoryByName(vBridgeName);
				if(gInventory.isEverythingFetched())// || (item_id.notNull() && isworn(item_id)))
				{
					//cmdline_printchat("inv is fetched");//<< llendl;
					
					if(item_id.notNull())
					{
						//cmdline_printchat("id="+item_id.asString());
						LLViewerInventoryItem* bridge = gInventory.getItem(item_id);
						if(bridge)
						{
							//cmdline_printchat("bridge is ready to attach");//<< llendl;
							if(isworn(bridge->getUUID()))
							{
								//cmdline_printchat("bridge is already worn");//<< llendl;
								sBridgeStatus = RECHAN;
							}else if(bridge->isComplete())
							{
								//cmdline_printchat("bridge is complete, attaching");//<< llendl;
								LLMessageSystem* msg = gMessageSystem;
								msg->newMessageFast(_PREHASH_RezSingleAttachmentFromInv);
								msg->nextBlockFast(_PREHASH_AgentData);
								msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
								msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
								msg->nextBlockFast(_PREHASH_ObjectData);
								msg->addUUIDFast(_PREHASH_ItemID, bridge->getUUID());
								msg->addUUIDFast(_PREHASH_OwnerID, bridge->getPermissions().getOwner());
								msg->addU8Fast(_PREHASH_AttachmentPt, 128);
								pack_permissions_slam(msg, bridge->getFlags(), bridge->getPermissions());
								msg->addStringFast(_PREHASH_Name, bridge->getName());
								msg->addStringFast(_PREHASH_Description, bridge->getDescription());
								msg->sendReliable(gAgent.getRegionHost());
								sBridgeStatus = RECHAN;
							}
						}
					}else
					{
						//cmdline_printchat("no bridge");//<< llendl;
						//sBridgeStatus = BUILDING;
						std::string directory = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"bridge.xml");
						if(!LLFile::isfile(directory.c_str()))
						{
							//cmdline_printchat("file not there o.o");//<< llendl;
							sBridgeStatus = FAILED;
						}else
						{
							//cmdline_printchat("bridge.xml located. importing..");//<< llendl;
							gImportTracker.importer(directory,&setBridgeObject);
							sBridgeStatus = BUILDING;
						}
					}
				}
			}
			break;
		case RENAMING:
			{
				////cmdline_printchat("renaming");
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_ObjectAttach);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addU8Fast(_PREHASH_AttachmentPoint, vCatType);
				
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addU32Fast(_PREHASH_ObjectLocalID, sBridgeObject->getLocalID());
				msg->addQuatFast(_PREHASH_Rotation, LLQuaternion(0.0f, 0.0f, 0.0f, 1.0f));
				
				msg->sendReliable(gAgent.getRegion()->getHost());
				sBridgeStatus = FOLDERING;
			}
			break;
		case FOLDERING:
			{
				////cmdline_printchat("foldering");
				LLUUID vcatid;
				vcatid = gInventory.findCategoryByName(vBridgeOpCat);
				if(vcatid.isNull())
				{
					////cmdline_printchat("creating folder");
					vcatid = gInventory.createNewCategory(gAgent.getInventoryRootID(), LLAssetType::AT_NONE, vBridgeOpCat);
				}
				LLUUID bridge_id = findInventoryByName(vBridgeName);
				//cmdline_printchat("id="+bridge_id.asString());
				LLViewerInventoryItem* bridge = gInventory.getItem(bridge_id);
				if(bridge)
				{
					move_inventory_item(gAgent.getID(),gAgent.getSessionID(),bridge->getUUID(),vcatid,vBridgeName, NULL);
					sBridgeStatus = RECHAN;
					////cmdline_printchat("moving to folder");
				}
			}
			break;
		case RECHAN:
			{
				{
					//if(l2c == 0) is this really needed ._. 
					//{
						send_chat_from_viewer("-1|l2c", CHAT_TYPE_WHISPER, JCLSLBridge::bridge_channel(gAgent.getID()));
						sBridgeStatus = ACTIVE;
					//}
				}
			}
		case ACTIVE:
			{
				LLUUID bridge = findInventoryByName(vBridgeName);
				//if(bridge)
				//LLVOAvatar* avatar = gAgent.getAvatarObject();
				if(bridge.isNull() || !isworn(bridge))
				{
					l2c = 0;
					l2c_inuse = false;
					////cmdline_printchat("reattaching");
					sBridgeStatus = UNINITIALIZED;
				}
			}
		}
	}
	return FALSE;
}
void JCLSLBridge::setBridgeObject(LLViewerObject* obj)
{
	sBridgeObject = obj;
	////cmdline_printchat("callback reached");
	sBridgeStatus = RENAMING;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectName);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_LocalID,obj->getLocalID());
	msg->addStringFast(_PREHASH_Name,vBridgeName);
	gAgent.sendReliableMessage();
}

void JCLSLBridge::callback_fire(U32 callback_id, LLSD data)
{
	if (!callback_id)
		return;

	std::map<U32, JCBridgeCallback*>::iterator i;

	i = callback_map.find(callback_id);
	if (i != callback_map.end())
	{
		(*i).second->fire(data);
		callback_map.erase(i);
	}
}

U32 JCLSLBridge::registerCB(JCBridgeCallback* cb)
{
	if (!cb)
		return 0;

	lastcall++;
	if (!lastcall)
		lastcall++;

	callback_map[lastcall] = cb;
	while(callback_map.size() > 100)callback_map.erase(0);
	return lastcall;
}

LLSD JCLSLBridge::parse_string_to_list(std::string list, char sep)
{
	LLSD newlist;
	U32 count = 0;
	std::string token;
	std::istringstream iss(list);
	while ( getline(iss, token, sep) )
	{
		newlist[count] = token;
		count += 1;
	}
	return newlist;
}


void JCLSLBridge::processSoundTrigger(LLMessageSystem* msg,void**)
{
	LLUUID	sound_id,owner_id;
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_SoundID, sound_id);
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_OwnerID, owner_id);
	if(owner_id == gAgent.getID())
	{
		
		if(sound_id == LLUUID("420d76ad-c82b-349d-7b81-f00d0ca0f38f"))
		{
			if(sBridgeStatus == ACTIVE)
			{
				send_chat_from_viewer("Meta7_bridge_rdy", CHAT_TYPE_WHISPER, JCLSLBridge::bridge_channel(gAgent.getID()));
			}else if(sBridgeStatus == FAILED)
			{
				send_chat_from_viewer("Meta7_bridge_failed", CHAT_TYPE_WHISPER, JCLSLBridge::bridge_channel(gAgent.getID()));
			}else
			{
				send_chat_from_viewer("Meta7_bridge_working", CHAT_TYPE_WHISPER, JCLSLBridge::bridge_channel(gAgent.getID()));
			}
		}
		
	}
}
/*
LLUUID BridgePermissions_Meta7("c78f9f3f-56ac-4442-a0b9-8b41dad455ae");

void JCLSLBridge::loadPermissions()
{
	std::vector<std::string> strings;
	strings.push_back( BridgePermissions_Meta7.asString() );//BridgePermissions Meta7
	send_generic_message("avatarnotesrequest", strings);
}

void JCLSLBridge::storePermissions()
{
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessage("AvatarNotesUpdate");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlock("Data");
	msg->addUUID("TargetID", BridgePermissions_Meta7);
	msg->addString("Notes", "");//RECHAN_permissions);
	gAgent.sendReliableMessage();
}
*/
void JCLSLBridge::processAvatarNotesReply(LLMessageSystem *msg, void**)
{
	// extract the agent id
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	LLUUID target_id;
	msg->getUUID("Data", "TargetID", target_id);

	/*if(target_id == BridgePermissions_Meta7)
	{
		std::string text;
		msg->getString("Data", "Notes", text);
		//LLSD arguments = JCLSLBridge::parse_string_to_list(text, '|');
		//RECHAN_permissions = text;//arguments;
	}*/
}
