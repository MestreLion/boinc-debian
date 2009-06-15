// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2008 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

#if defined(_WIN32) && !defined(__STDWX_H__) && !defined(_BOINC_WIN_) && !defined(_AFX_STDAFX_H_)
#include "boinc_win.h"
#endif

#ifndef _WIN32
#include "config.h"
#include <cstring>
#include <string>
#endif

using std::string;

#include "parse.h"
#include "error_numbers.h"
#include "proxy_info.h"

int PROXY_INFO::parse(MIOFILE& in) {
    char buf[1024];
    string s5un, s5up, hun, hup, temp;

    memset(this, 0, sizeof(PROXY_INFO));
    while (in.fgets(buf, 256)) {
        if (match_tag(buf, "</proxy_info>")) return 0;
        else if (match_tag(buf, "<use_http_proxy/>")) use_http_proxy = true;
        else if (match_tag(buf, "<use_socks_proxy/>")) use_socks_proxy = true;
        else if (match_tag(buf, "<use_http_auth/>")) use_http_auth = true;
        else if (parse_int(buf, "<socks_version>", socks_version)) continue;
        else if (parse_str(buf, "<socks_server_name>", socks_server_name, sizeof(socks_server_name))) continue;
        else if (parse_int(buf, "<socks_server_port>", socks_server_port)) continue;
        else if (parse_str(buf, "<http_server_name>", http_server_name, sizeof(http_server_name))) continue;
        else if (parse_int(buf, "<http_server_port>", http_server_port)) continue;
        else if (parse_str(buf, "<socks5_user_name>", socks5_user_name,sizeof(socks5_user_name))) continue;
        else if (parse_str(buf, "<socks5_user_passwd>", socks5_user_passwd,sizeof(socks5_user_passwd))) continue;
        else if (parse_str(buf, "<http_user_name>", http_user_name,sizeof(http_user_name))) continue;
        else if (parse_str(buf, "<http_user_passwd>", http_user_passwd,sizeof(http_user_passwd))) continue;
		else if (parse_str(buf, "<no_proxy>", noproxy_hosts, sizeof(noproxy_hosts))) continue;
    }
    return ERR_XML_PARSE;
}

int PROXY_INFO::write(MIOFILE& out) {
    char s5un[2048], s5up[2048], hun[2048], hup[2048];
    xml_escape(socks5_user_name, s5un, sizeof(s5un));
    xml_escape(socks5_user_passwd, s5up, sizeof(s5up));
    xml_escape(http_user_name, hun, sizeof(hun));
    xml_escape(http_user_passwd, hup, sizeof(hup));
    out.printf(
        "<proxy_info>\n"
        "%s"
        "%s"
        "%s"
        "    <socks_version>%d</socks_version>\n"
        "    <socks_server_name>%s</socks_server_name>\n"
        "    <socks_server_port>%d</socks_server_port>\n"
        "    <http_server_name>%s</http_server_name>\n"
        "    <http_server_port>%d</http_server_port>\n"
        "    <socks5_user_name>%s</socks5_user_name>\n"
        "    <socks5_user_passwd>%s</socks5_user_passwd>\n"
        "    <http_user_name>%s</http_user_name>\n"
        "    <http_user_passwd>%s</http_user_passwd>\n"
		"    <no_proxy>%s</no_proxy>\n"
		"</proxy_info>\n",        
        use_http_proxy?"    <use_http_proxy/>\n":"",
        use_socks_proxy?"    <use_socks_proxy/>\n":"",
        use_http_auth?"    <use_http_auth/>\n":"",
        socks_version,
        socks_server_name,
        socks_server_port,
        http_server_name,
        http_server_port,
        s5un,
        s5up,
        hun,
        hup,
		noproxy_hosts
    );
    return 0;
}

void PROXY_INFO::clear() {
    use_http_proxy = false;
    use_socks_proxy = false;
    use_http_auth = false;
    strcpy(socks_server_name, "");
    strcpy(http_server_name, "");
    socks_server_port = 80;
    http_server_port = 80;
    strcpy(socks5_user_name, "");
    strcpy(socks5_user_passwd, "");
    strcpy(http_user_name, "");
    strcpy(http_user_passwd, "");
    socks_version = 0;
	strcpy(noproxy_hosts, "");
}

const char *BOINC_RCSID_af13db88e5 = "$Id: proxy_info.cpp 16069 2008-09-26 18:20:24Z davea $";