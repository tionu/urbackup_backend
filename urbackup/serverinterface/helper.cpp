/*************************************************************************
*    UrBackup - Client/Server backup system
*    Copyright (C) 2011  Martin Raiber
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef CLIENT_ONLY

#include "helper.h"
#include "../../stringtools.h"
#include "../database.h"

Helper::Helper(THREAD_ID pTID, str_map *pGET, str_nmap *pPARAMS)
{
	session=NULL;
	update(pTID,pGET,pPARAMS);
}

void Helper::update(THREAD_ID pTID, str_map *pGET, str_nmap *pPARAMS)
{
	tid=pTID;
	GET=pGET;
	PARAMS=pPARAMS;

	if( session==NULL )
	{	
		session=Server->getSessionMgr()->getUser( (*GET)[L"ses"], widen((*PARAMS)["REMOTE_ADDR"]+(*PARAMS)["HTTP_USER_AGENT"]) );
	}

	//Get language from ACCEPT_LANGUAGE
	str_map::iterator lit=GET->find(L"lang");
	if(lit!=pGET->end())
	{
		language=wnarrow(lit->second);
	}
	else
	{
		str_nmap::iterator al=PARAMS->find("ACCEPT_LANGUAGE");
		if(al!=PARAMS->end())
		{
			std::vector<std::string> toks;
			Tokenize(al->second, toks, ",");
			for(size_t i=0;i<toks.size();++i)
			{
				std::string lstr=getuntil(";", toks[i]);
				if(lstr.empty())
					lstr=toks[i];

				std::string prefix=getuntil("-", lstr);
				if(prefix.empty())
					prefix=lstr;

				if(i==0)
				{
					language=strlower(prefix);
				}
			}
		}
		else
		{
			language="en";
		}
	}

	if( session==NULL)
		invalid_session=true;
	else
		invalid_session=false;
}

SUser *Helper::getSession(void)
{
	return session;
}

void Helper::OverwriteLanguage( std::string pLanguage)
{
	language=pLanguage;
}

ITemplate *Helper::createTemplate(std::string name)
{
	IDatabase* db=NULL;//Server->getDatabase(tid, TRANSLATIONDB);

	ITemplate *tmpl=Server->createTemplate("urbackup/templates/"+name);

	if( db!=NULL )
	{
		tmpl->addValueTable(db, "translation_"+language );
	}

	if( invalid_session==true )
		tmpl->setValue(L"INVALID_SESSION",L"true");
	else if(session!=NULL)
		tmpl->setValue(L"SESSION", session->session);

	if( session!=NULL && session->id==-1 )
		tmpl->setValue(L"INVALID_ID",L"true");

	templates.push_back( tmpl );

	return tmpl;
}

Helper::~Helper(void)
{
	if( session!=NULL )
		Server->getSessionMgr()->releaseUser(session);

	for(size_t i=0;i<templates.size();++i)
	{
		Server->destroy( templates[i] );
	}
}

void Helper::Write(std::string str)
{
	Server->Write( tid, str );
}

void Helper::WriteTemplate(ITemplate *tmpl)
{
	Server->Write( tid, tmpl->getData() );
}

IDatabase *Helper::getDatabase(void)
{
	return Server->getDatabase(tid, URBACKUPDB_SERVER);
}

std::wstring Helper::generateSession(std::wstring username)
{
	return Server->getSessionMgr()->GenerateSessionIDWithUser( username, widen((*PARAMS)["REMOTE_ADDR"]+(*PARAMS)["HTTP_USER_AGENT"]) );
}

std::string Helper::getRights(const std::string &domain)
{
	if(session==NULL) return "none";
	if(session->id==0) return "all";

	if(getRightsInt("all")=="all")
		return "all";

	return getRightsInt(domain);
}

std::string Helper::getRightsInt(const std::string &domain)
{
	if(session==NULL) return "none";

	IQuery *q=getDatabase()->Prepare("SELECT t_right FROM si_permissions WHERE clientid=? AND t_domain=?");
	q->Bind(session->id);
	q->Bind(domain);
	db_results res=q->Read();
	q->Reset();
	if(!res.empty())
	{
		return wnarrow(res[0][L"t_right"]);
	}
	else
	{
		return "none";
	}
}

void Helper::releaseAll(void)
{
	if(session!=NULL)
	{
		Server->getSessionMgr()->releaseUser(session);
		session=NULL;
	}
}

std::string Helper::getTimeFormatString(void)
{
	if(language=="de")
	{
		return "%d.%m.%Y %H:%M";
	}
	else
	{
		return "%m/%d/%Y %H:%M";
	}
}

std::string Helper::getLanguage(void)
{
	return language;
}

#endif //CLIENT_ONLY