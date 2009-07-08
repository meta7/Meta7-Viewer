/* Copyright (c) 2009
 *
 * Modular Systems Ltd. All rights reserved.
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
 *   3. Neither the name Modular Systems Ltd nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS LTD AND CONTRIBUTORS �AS IS�
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

#include "chatbar_as_cmdline.h"

#include "llcalc.h"

#include "llchatbar.h"
#include "llagent.h"
#include "stdtypes.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "lluuid.h"
#include "llviewercontrol.h"

#include "material_codes.h"
#include "llvolume.h"
#include "object_flags.h"
#include "llvolumemessage.h"
#include "llurldispatcher.h"
#include "llworldmap.h"

#include <iosfwd>

#include <float.h>

#include "llchat.h"

#include "llfloaterchat.h"

//void printchat(std::string message);


bool cmd_line_chat(std::string revised_text, EChatType type)
{
	if(gSavedSettings.getBOOL("EmeraldCmdLine"))
	{
		std::istringstream i(revised_text);
		std::string command;
		i >> command;
		if(command != "")
		{
			if(command == gSavedSettings.getString("EmeraldCmdLinePos"))
			{
				F32 x,y,z;
				if (i >> x)
				{
					if (i >> y)
					{
						if (i >> z)
						{
							LLVector3 agentPos = gAgent.getPositionAgent();
							LLViewerRegion* agentRegion = gAgent.getRegion();
							if(agentRegion)
							{
								LLVector3 targetPos(x,y,z);
								gAgent.teleportRequest(agentRegion->getHandle(),targetPos);
								return false;
							}
						}
					}
				}
			}
			else if(command == gSavedSettings.getString("EmeraldCmdLineDrawDistance"))
			{
                std::istringstream i(revised_text);
                std::string command;
                i >> command;
                int drawDist;
                if(i >> drawDist)
                {
                    gSavedSettings.setF32("RenderFarClip", drawDist);
                    gAgent.mDrawDistance=drawDist;
                    LLChat chat;
                    char buffer[DB_IM_MSG_BUF_SIZE * 2];  /* Flawfinder: ignore */
                    snprintf(buffer,sizeof(buffer),"Draw distance set to: %dm",drawDist);
                    chat.mText = buffer;
                    LLFloaterChat::addChat(chat, FALSE, FALSE);
					return false;
                }
			}
			else if(command == gSavedSettings.getString("EmeraldCmdTeleportToCam"))
            {
				gAgent.teleportViaLocation(gAgent.getCameraPositionGlobal());
				return false;
            }
			else if(command == gSavedSettings.getString("EmeraldCmdLineKeyToName"))
            {
                std::istringstream istream(revised_text);
                std::string command;
                istream >> command;
                LLUUID targetKey;
                if(istream >> targetKey)
                {
                    std::string object_name;
                    gCacheName->getFullName(targetKey, object_name);
                    LLChat chat;
                    char buffer[DB_IM_MSG_BUF_SIZE * 2];  /* Flawfinder: ignore */
                    snprintf(buffer,sizeof(buffer),"%s: (%s)",targetKey.asString().c_str(), object_name.c_str());
                    chat.mText = buffer;
                    LLFloaterChat::addChat(chat, FALSE, FALSE);
                }
				return false;
            }
			else if(command == gSavedSettings.getString("EmeraldCmdLineOfferTp"))
            {
                std::istringstream istream(revised_text);
                std::string command;
                istream >> command;
                std::string avatarName;
//				llinfos << "CMD DEBUG 0 " << command << " " << avatarName << llendl;
                if(istream >> avatarName)
                {
//				llinfos << "CMD DEBUG 0 afterif " << command << " " << avatarName << llendl;
                    LLUUID tempUUID;
                    if(LLUUID::parseUUID(avatarName, &tempUUID))
                    {
                        LLChat chat;
                        char buffer[DB_IM_MSG_BUF_SIZE * 2];  /* Flawfinder: ignore */
                        LLDynamicArray<LLUUID> ids;
                        ids.push_back(tempUUID);
                        std::string tpMsg="Join me!";
                        LLMessageSystem* msg = gMessageSystem;
                        msg->newMessageFast(_PREHASH_StartLure);
                        msg->nextBlockFast(_PREHASH_AgentData);
                        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
                        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
                        msg->nextBlockFast(_PREHASH_Info);
                        msg->addU8Fast(_PREHASH_LureType, (U8)0); 

                        msg->addStringFast(_PREHASH_Message, tpMsg);
                        for(LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end(); ++itr)
                        {
                            msg->nextBlockFast(_PREHASH_TargetData);
                            msg->addUUIDFast(_PREHASH_TargetID, *itr);
                        }
                        gAgent.sendReliableMessage();
                        ids=NULL;
                        snprintf(buffer,sizeof(buffer),"Offered TP to key %s",tempUUID.asString().c_str());
                        chat.mText = buffer;
                        LLFloaterChat::addChat(chat, FALSE, FALSE);
						return false;
                    }
                }
            }
			
			else if(command == gSavedSettings.getString("EmeraldCmdLineGround"))
			{
				LLVector3 agentPos = gAgent.getPositionAgent();
				U64 agentRegion = gAgent.getRegion()->getHandle();
				LLVector3 targetPos(agentPos.mV[0],agentPos.mV[1],LLWorld::getInstance()->resolveLandHeightAgent(agentPos));
				gAgent.teleportRequest(agentRegion,targetPos);
				return false;
			}else if(command == gSavedSettings.getString("EmeraldCmdLineHeight"))
			{
				F32 z;
				if(i >> z)
				{
					LLVector3 agentPos = gAgent.getPositionAgent();
					U64 agentRegion = gAgent.getRegion()->getHandle();
					LLVector3 targetPos(agentPos.mV[0],agentPos.mV[1],z);
					gAgent.teleportRequest(agentRegion,targetPos);
					return false;
				}
			}else if(command == gSavedSettings.getString("EmeraldCmdLineTeleportHome"))
			{
				gAgent.teleportHome();
				return false;
            }else if(command == gSavedSettings.getString("EmeraldCmdLineRezPlatform"))
            {
                LLVector3 agentPos = gAgent.getPositionAgent()+(gAgent.getVelocity()*(F32)0.333);
                LLMessageSystem* msg = gMessageSystem;
                msg->newMessageFast(_PREHASH_ObjectAdd);
                msg->nextBlockFast(_PREHASH_AgentData);
                msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
                msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
                msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
                msg->nextBlockFast(_PREHASH_ObjectData);
                msg->addU8Fast(_PREHASH_PCode, LL_PCODE_VOLUME);
                msg->addU8Fast(_PREHASH_Material,    LL_MCODE_METAL);

                if(agentPos.mV[2] > 4096.0)msg->addU32Fast(_PREHASH_AddFlags, FLAGS_CREATE_SELECTED);
                else msg->addU32Fast(_PREHASH_AddFlags, 0);

                LLVolumeParams    volume_params;

                volume_params.setType( LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_CIRCLE2 );
                volume_params.setRatio    ( 2, 2 );
                volume_params.setShear    ( 0, 0 );
                volume_params.setTaper(2.0f,2.0f);
                volume_params.setTaperX(0.f);
                volume_params.setTaperY(0.f);

                LLVolumeMessage::packVolumeParams(&volume_params, msg);
                LLVector3 rezpos = agentPos - LLVector3(0.0f,0.0f,2.5f);
                LLQuaternion rotation;
                rotation.setQuat(90.f * DEG_TO_RAD, LLVector3::y_axis);

                msg->addVector3Fast(_PREHASH_Scale,            LLVector3(0.01f,10.0f,10.0f) );
                msg->addQuatFast(_PREHASH_Rotation,            rotation );
                msg->addVector3Fast(_PREHASH_RayStart,        rezpos );
                msg->addVector3Fast(_PREHASH_RayEnd,            rezpos );
                msg->addU8Fast(_PREHASH_BypassRaycast,        (U8)1 );
                msg->addU8Fast(_PREHASH_RayEndIsIntersection, (U8)FALSE );
                msg->addU8Fast(_PREHASH_State, 0);
                msg->addUUIDFast(_PREHASH_RayTargetID,            LLUUID::null );
                msg->sendReliable(gAgent.getRegionHost());
				return false;
			}else if(command == gSavedSettings.getString("EmeraldCmdLineMapTo"))
			{
				LLVector3d agentPos = gAgent.getPositionGlobal();
				S32 agent_x = llround( (F32)fmod( agentPos.mdV[VX], (F64)REGION_WIDTH_METERS ) );
				S32 agent_y = llround( (F32)fmod( agentPos.mdV[VY], (F64)REGION_WIDTH_METERS ) );
				S32 agent_z = llround( (F32)agentPos.mdV[VZ] );
				std::string region_name = LLWeb::escapeURL(revised_text.substr(command.length()+1));
				std::string url;

				if(!gSavedSettings.getBOOL("EmeraldMapToKeepPos"))
				{
					agent_x = 128;
					agent_y = 128;
					agent_z = 0;
				}

				url = llformat("secondlife:///app/teleport/%s/%d/%d/%d",region_name.c_str(),agent_x,agent_y,agent_z);
				LLURLDispatcher::dispatch(url, NULL, true);
				return false;
			}else if(command == gSavedSettings.getString("EmeraldCmdLineCalc"))//Cryogenic Blitz
			{
				bool success;
				F32 result = 0.f;
				std::string expr = revised_text.substr(command.length()+1);
				LLStringUtil::toUpper(expr);
				success = LLCalc::getInstance()->evalString(expr, result);

				LLChat chat;

				if (!success)
				{
					chat.mText =  "Calculation Failed";
				}
				else
				{
					// Replace the expression with the result
					std::ostringstream result_str;
					result_str << expr;
					result_str << " = ";
					result_str << result;
					chat.mText = result_str.str();
				}
				chat.mSourceType = CHAT_SOURCE_SYSTEM;
				LLFloaterChat::addChat(chat);
				return false;
			}
		}
	}
	return true;
}

