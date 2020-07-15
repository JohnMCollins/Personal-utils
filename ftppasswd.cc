//
// Copyright (c) Xi Software Ltd. 2005.
//
// ftppasswd.cc: created by John M Collins on Sat Jan 15 2005.
//----------------------------------------------------------------------
// $Header$
// $Log$
//----------------------------------------------------------------------
// C++ Version:

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <libxml/parser.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
//#if	defined(i386) && defined(__GNUC__)
//#define	NO_ASM
//#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef	AF_INET
#define	AF_INET	2
#endif
#include <signal.h>
#include <string.h>
#include <errno.h>

using  namespace  std;

class	xml_fd  {
protected:
	xmlDocPtr	doc;
	xmlNodePtr	root;
	xmlNsPtr	ns;

public:
	xml_fd() : doc(0), root(0), ns(0)  { }
	~xml_fd()  {  if  (doc)  xmlFreeDoc(doc);  }

	void	initialise(const int = -1);
	bool	start_load(const string &, const char *, const char *);
	bool	start_memload(const string &);
	void	finish_load();
	void	start_save(const char *, const char *, const char *);
	xmlNodePtr  start_memsave(const char *);
	bool	finish_save(const string &);
	bool	finish_memsave(string &);

	xmlNodePtr  get_root_element() const  {  return  root;  }

	xmlNodePtr  save_xmlchild(xmlNodePtr, const char *);
	xmlNodePtr  save_xmltextchild(xmlNodePtr, const char *, const char *);
	xmlNodePtr  save_xmltextchild(xmlNodePtr nd, const char *n, const string &t)  {  return save_xmltextchild(nd, n, t.c_str());  }

	xmlNodePtr  new_or_replace(xmlNodePtr, const char *, xmlNodePtr);

	void	save_stringvalue(xmlNodePtr, const char *, const char *);
	void	save_stringvalue(xmlNodePtr nd, const char *n, const string &s)  { save_stringvalue(nd, n, s.c_str());  }
	void	save_intvalue(xmlNodePtr, const char *, const int);
	void	save_unsignedvalue(xmlNodePtr, const char *, const unsigned);
	void	save_timevalue(xmlNodePtr, const char *, const time_t);

	xmlNodePtr	save_namecontext(xmlNodePtr, const char *);

	void	save_cdata(xmlNodePtr, const char *, const string &);

	xmlNodePtr  findnode(xmlNodePtr, const char *);
	xmlNodePtr  findnodeprop(xmlNodePtr, const char *, const char *, const char *);

	bool	get_stringvalue(xmlNodePtr, string &);
	bool	get_intvalue(xmlNodePtr, int &);
	bool	get_unsignedvalue(xmlNodePtr, unsigned &);
	bool	get_timevalue(xmlNodePtr, time_t &);

	bool	load_stringvalue(xmlNodePtr, const char *, string &);
	bool	load_stringvalue_nullok(xmlNodePtr, const char *, string &);
	bool	load_intvalue(xmlNodePtr, const char *, int &);
	bool	load_unsignedvalue(xmlNodePtr, const char *, unsigned &);
	bool	load_timevalue(xmlNodePtr, const char *, time_t &);
};

namespace  xml_funcs  {

	//  Standard utils

	extern	void	save_stringprop(xmlNodePtr, const char *, const char *);
	inline	void	save_stringprop(xmlNodePtr nd, const char *n, const string &s)  { save_stringprop(nd, n, s.c_str());  }
	extern	void	save_intprop(xmlNodePtr, const char *, const int);
	extern	void	save_unsignedprop(xmlNodePtr, const char *, const unsigned);
	extern 	void	save_timeprop(xmlNodePtr, const char *, const time_t);
	extern	void	save_boolprop(xmlNodePtr, const char *, const bool);

	//  Load utils

	extern	bool	load_stringprop(xmlNodePtr, const char *, string &);
	extern	bool	load_intprop(xmlNodePtr, const char *, int &);
	extern	bool	load_unsignedprop(xmlNodePtr, const char *, unsigned &);
	extern	bool	load_timeprop(xmlNodePtr, const char *, time_t &);
	extern	bool	load_boolprop(xmlNodePtr, const char *);
}

void	xml_fd::initialise(const int cm)
{
	xmlKeepBlanksDefault(0);
}

bool	xml_fd::start_load(const string &fname, const char *uri, const char *lname)
{
	if  (!(doc = xmlParseFile(fname.c_str())))
		return  false;

	if  (!(root = xmlDocGetRootElement(doc)))  {
		xmlFreeDoc(doc);
		doc = 0;
		return  false;
	}

	ns = xmlSearchNsByHref(doc, root, (const xmlChar *) uri);
	if  (!ns  ||  strcmp((const char *) root->name, lname) != 0)  {
		xmlFreeDoc(doc);
		doc = 0;
		return  false;
	}
	return  true;
}

//  Variant for memory-parsing (for sched requests etc).

bool	xml_fd::start_memload(const string &xml_req)
{
	if  (!(doc = xmlParseMemory(xml_req.c_str(), int(xml_req.length()))))
		return  false;
	if  (!(root = xmlDocGetRootElement(doc)))  {
		xmlFreeDoc(doc);
		doc = 0;
		return  false;
	}
	return  true;
}

void	xml_fd::finish_load()
{
	if  (doc)  {
		xmlFreeDoc(doc);
		doc = 0;
	}
}

void	xml_fd::start_save(const char *savename, const char *uri, const char *lname)
{
	doc = xmlNewDoc((const xmlChar *) "1.0");
	root = xmlNewDocNode(doc, 0, (const xmlChar *) savename, 0);
	xmlDocSetRootElement(doc, root);
	ns = xmlNewNs(root, (const xmlChar *) uri, (const xmlChar *) lname);
	xmlSetNs(root, ns);
}

xmlNodePtr  xml_fd::start_memsave(const char *savename)
{
	doc = xmlNewDoc((const xmlChar *) "1.0");
	root = xmlNewDocNode(doc, 0, (const xmlChar *) savename, 0);
	xmlDocSetRootElement(doc, root);
	return  root;
}

bool	xml_fd::finish_save(const string &fname)
{
	const  char  wr[] = "writer";
	xmlNodePtr namenode = xmlNewNode(ns, (const xmlChar *) wr);
	new_or_replace(root, wr, namenode);
	int	res = xmlSaveFormatFile(fname.c_str(), doc, 1);
	xmlFreeDoc(doc);
	doc = 0;
	return  res >= 0;
}

bool	xml_fd::finish_memsave(string &result)
{
	xmlChar  *res;
	int	sz;
	xmlDocDumpMemory(doc, &res, &sz);
	xmlFreeDoc(doc);
	doc = 0;
	if  (sz <= 0)
		return  false;
	result = string(reinterpret_cast<const char *>(res), size_t(sz));
	xmlFree(res);
	return  true;
}

xmlNodePtr  xml_fd::save_xmlchild(xmlNodePtr parent, const char *nam)
{
	return  xmlNewChild(parent, ns, (const xmlChar *) nam, 0);
}

xmlNodePtr  xml_fd::save_xmltextchild(xmlNodePtr parent, const char *nam, const char *txt)
{
	return  xmlNewTextChild(parent, ns, (const xmlChar *) nam, (const xmlChar *) txt);
}

xmlNodePtr  xml_fd::new_or_replace(xmlNodePtr parent, const char *nm, xmlNodePtr newnd)
{
	if  (xmlNodePtr  oldnode = findnode(parent, nm))  {
		xmlReplaceNode(oldnode, newnd);
		xmlFreeNode(oldnode);
	}
	else
		xmlAddChild(parent, newnd);
	return  newnd;
}

void	xml_fd::save_stringvalue(xmlNodePtr nd, const char *nm, const char *str)
{
	xmlNewTextChild(nd, ns, (const xmlChar *) nm, (const xmlChar *) str);
}

void	xml_fd::save_intvalue(xmlNodePtr nd, const char *nm, const int val)
{
	ostringstream  ostr;
	ostr << val;
	save_stringvalue(nd, nm, ostr.str().c_str());
}

void	xml_fd::save_unsignedvalue(xmlNodePtr nd, const char *nm, const unsigned val)
{
	ostringstream  ostr;
	ostr << val;
	save_stringvalue(nd, nm, ostr.str().c_str());
}

void	xml_fd::save_timevalue(xmlNodePtr nd, const char *nm, const time_t val)
{
	ostringstream  ostr;
	ostr << val;
	save_stringvalue(nd, nm, ostr.str().c_str());
}

xmlNodePtr  xml_fd::save_namecontext(xmlNodePtr parent, const char *name)
{
	xmlNodePtr  result = save_xmlchild(parent, "context");
	xmlSetProp(result, (const xmlChar *) "name", (const xmlChar *) name);
	return  result;
}

void	xml_fd::save_cdata(xmlNodePtr parent, const char *name, const string &data)
{
	xmlNodePtr  cdn = save_xmlchild(parent, name);
	xmlNodePtr  cd = xmlNewCDataBlock(doc, reinterpret_cast<xmlChar *>(const_cast<char *>(data.c_str())), data.length());
	xmlAddChild(cdn, cd);
}

xmlNodePtr  xml_fd::findnode(xmlNodePtr nd, const char *name)
{
	for  (xmlNodePtr sub = nd->children;  sub;  sub = sub->next)
		if  (sub->ns == ns  &&  strcmp((const char *) sub->name, name) == 0)
			return  sub;
	return  0;
}

xmlNodePtr  xml_fd::findnodeprop(xmlNodePtr nd, const char *name, const char *propname, const char *propval)
{
	for  (xmlNodePtr sub = nd->children;  sub;  sub = sub->next)
		if  (sub->ns == ns  &&  strcmp((const char *) sub->name, name) == 0)  {
			const char *pv = (const char *) xmlGetProp(sub, (const xmlChar *) propname);
			if  (pv)  {
				if  (strcmp(pv, propval) == 0)  {
					xmlFree((xmlChar *) pv);
					return  sub;
				}
				xmlFree((xmlChar *) pv);
			}
		}
	return  0;
}

bool	xml_fd::get_stringvalue(xmlNodePtr nd, string &result)
{
	xmlChar  *ret = xmlNodeListGetString(doc, nd->children, 1);
	if  (ret)  {
		result = (const char *) ret;
		xmlFree(ret);
		return  true;
	}
	return  false;
}

bool	xml_fd::get_intvalue(xmlNodePtr nd, int &result)
{
	xmlChar  *ret = xmlNodeListGetString(doc, nd->children, 0);
	if  (ret)  {
		char *endp;
		int val = strtol((const char *) ret, &endp, 0);
		if  (val == 0  &&  (const char *) ret == endp)  {
			xmlFree(ret);
			return  false;
		}
		xmlFree(ret);
		result = val;
		return  true;
	}
	return  false;
}

bool	xml_fd::get_unsignedvalue(xmlNodePtr nd, unsigned &result)
{
	xmlChar  *ret = xmlNodeListGetString(doc, nd->children, 0);
	if  (ret)  {
		char *endp;
		unsigned val = strtoul((const char *) ret, &endp, 0);
		if  (val == 0  &&  (const char *) ret == endp)  {
			xmlFree(ret);
			return  false;
		}
		xmlFree(ret);
		result = val;
		return  true;
	}
	return  false;
}

bool	xml_fd::get_timevalue(xmlNodePtr nd, time_t &result)
{
	xmlChar  *ret = xmlNodeListGetString(doc, nd->children, 0);
	if  (ret)  {
		char *endp;
		time_t val = strtoul((const char *) ret, &endp, 0);
		if  (val == 0  &&  (const char *) ret == endp)  {
			xmlFree(ret);
			return  false;
		}
		xmlFree(ret);
		result = val;
		return  true;
	}
	return  false;
}


bool	xml_fd::load_stringvalue(xmlNodePtr nd, const char *name, string &result)
{
	if  (xmlNodePtr  cnd = findnode(nd, name))
		return  get_stringvalue(cnd, result);
	return  false;
}

bool	xml_fd::load_stringvalue_nullok(xmlNodePtr nd, const char *name, string &result)
{
	if  (xmlNodePtr  cnd = findnode(nd, name))  {
		if  (!get_stringvalue(cnd, result))
			result = "";
		return  true;
	}
	return  false;
}

bool	xml_fd::load_intvalue(xmlNodePtr nd, const char *name, int &result)
{
	if  (xmlNodePtr  cnd = findnode(nd, name))
		return  get_intvalue(cnd, result);
	return  false;
}

bool	xml_fd::load_unsignedvalue(xmlNodePtr nd, const char *name, unsigned &result)
{
	if  (xmlNodePtr  cnd = findnode(nd, name))
		return  get_unsignedvalue(cnd, result);
	return  false;
}

bool	xml_fd::load_timevalue(xmlNodePtr nd, const char *name, time_t &result)
{
	if  (xmlNodePtr  cnd = findnode(nd, name))
		return  get_timevalue(cnd, result);
	return  false;
}

namespace  xml_funcs  {

	void	save_stringprop(xmlNodePtr nd, const char *name, const char *value)
	{
		xmlSetProp(nd, (const xmlChar *) name, (const xmlChar *) value);
	}

	void	save_intprop(xmlNodePtr nd, const char *name, const int value)
	{
		ostringstream  ostr;
		ostr << value;
		save_stringprop(nd, name, ostr.str().c_str());
	}

	void	save_unsignedprop(xmlNodePtr nd, const char *name, const unsigned value)
	{
		ostringstream  ostr;
		ostr << value;
		save_stringprop(nd, name, ostr.str().c_str());
	}

	void	save_timeprop(xmlNodePtr nd, const char *name, const time_t value)
	{
		ostringstream  ostr;
		ostr << value;
		save_stringprop(nd, name, ostr.str().c_str());
	}

	void	save_boolprop(xmlNodePtr nd, const char *name, const bool value)
	{
		if  (value)
			save_stringprop(nd, name, "y");
	}

	bool	load_stringprop(xmlNodePtr nd, const char *nm, string &result)
	{
		char *pv = (char *) xmlGetProp(nd, (const xmlChar *) nm);
		if  (pv)  {
			result = pv;
			xmlFree((xmlChar *) pv);
			return  true;
		}
		return  false;
	}

	bool	load_intprop(xmlNodePtr nd, const char *nm, int &result)
	{
		char *pv = (char *) xmlGetProp(nd, (const xmlChar *) nm);
		if  (pv)  {
			char  *endp;
			int val = strtol(pv, &endp, 0);
			if  (val == 0  &&  pv == endp)  {
				xmlFree((xmlChar *) pv);
				return  false;
			}
			result = val;
			xmlFree((xmlChar *) pv);
			return  true;
		}
		return  false;
	}

	bool	load_unsignedprop(xmlNodePtr nd, const char *nm, unsigned &result)
	{
		char *pv = (char *) xmlGetProp(nd, (const xmlChar *) nm);
		if  (pv)  {
			char  *endp;
			unsigned val = strtoul(pv, &endp, 0);
			if  (val == 0  &&  pv == endp)  {
				xmlFree((xmlChar *) pv);
				return  false;
			}
			result = val;
			xmlFree((xmlChar *) pv);
			return  true;
		}
		return  false;
	}

	bool	load_timeprop(xmlNodePtr nd, const char *nm, time_t &result)
	{
		char *pv = (char *) xmlGetProp(nd, (const xmlChar *) nm);
		if  (pv)  {
			char  *endp;
			int val = strtol(pv, &endp, 0);
			if  (val == 0  &&  pv == endp)  {
				xmlFree((xmlChar *) pv);
				return  false;
			}
			result = val;
			xmlFree((xmlChar *) pv);
			return  true;
		}
		return  false;
	}

	bool	load_boolprop(xmlNodePtr nd, const char *nm)
	{
		xmlChar *pv = xmlGetProp(nd, (const xmlChar *) nm);
		if  (pv)  {
			xmlFree(pv);
			return  true;
		}
		return  false;
	}
}

class	error_report  {
public:
	string	msg;
	const  int  code;
	error_report(const string &m, const int c) : msg(m), code(c)  { }
	void	stderr_report();
	void	xml_report(const int);
};

void	error_report::stderr_report()
{
	cerr << msg << endl;
}

string	generate_passwd(const char *dicname)
{
	ifstream  dic(dicname);

	if  (!dic.good())  {
		ostringstream  ostr;
		ostr << "Could not open dictionary " << dicname;
		throw  error_report(ostr.str(), 2);
	}

	vector<long>  starts;
	while  (dic)  {
		starts.push_back(dic.tellg());
		string  s;
		getline(dic, s);
		if  (s.length() < 2)
			starts.pop_back();
	}

	dic.clear();
	int nwords = 1 + random() % 3;
	string  result;
	for  (int cnt = 0;  cnt < nwords;  cnt++)  {
		dic.seekg(starts[random() % starts.size()]);
		string  s;
		getline(dic, s);
		if  (result.length() > 0)
			result += ' ';
		result += s;
	}
	return  result;
}

string	crypt_passwd(char *pw)
{
	string  salt = "$1$";
	salt += l64a(random());
	salt += '$';
	return  crypt(pw, salt.c_str());
}

bool	findu(string user, const char *fname)
{
	ifstream  fil(fname);

	while  (fil)  {
		string	s;
		getline(fil, s);
		size_t  cp = s.find_first_of(':');
		if  (cp != string::npos  &&  s.substr(0, cp) == user)
			return  true;
	}
	return  false;
}

string	gethomed(string user, const char *fname)
{
	ifstream  fil(fname);

	while  (fil)  {
		string	s;
		getline(fil, s);
		size_t  cp = s.find_first_of(':');
		if  (cp != string::npos  &&  s.substr(0, cp) == user)  {
			for  (int n = 1;  n < 5;  n++)
				cp = s.find_first_of(':', cp+1);
			size_t  ep = s.find_first_of(':', cp+1);
			return  s.substr(cp+1, ep-cp-1);
		}
	}
	return  "/";
}

void	replacepw(string user, const char *fname, string pw)
{
	string	ftl = fname;
	ftl += '-';
	ifstream fil(fname);
	ofstream tfile(ftl.c_str());
	if  (!tfile)  {
		ostringstream ostr;
		ostr << "Could not open temp file " << ftl << " error was " << strerror(errno);
		throw error_report(ostr.str(), 6);
	}
	while  (fil)  {
		string	s;
		getline(fil, s);
		size_t  cp = s.find_first_of(':');
		if  (cp != string::npos  &&  s.substr(0, cp) == user)  {
			size_t  ep = s.find_first_of(':', cp+1);
			tfile << user << ':' << crypt_passwd((char *) pw.c_str()) << s.substr(ep) << endl;
		}
		else
			tfile << s << endl;
	}

	fil.close();
	tfile.close();
	ifstream  itfile(ftl.c_str());
	ofstream  ofil(fname);
	while  (itfile)  {
		string  s;
		getline(itfile, s);
		if  (s.length() > 0)
			ofil << s << endl;
	}
	unlink(ftl.c_str());
}

void	insertpw(string user, const char *fname, string pw)
{
	passwd  *pent = getpwnam(user.c_str());
	if  (!pent)  {
		ostringstream  ostr;
		ostr << "Unknown user " << user;
		throw  error_report(ostr.str(), 5);
	}
	ofstream outp(fname, ios_base::app);
	outp << user << ':' << crypt_passwd((char *) pw.c_str()) << ':' << pent->pw_uid << ':' << pent->pw_gid << ':' << pent->pw_gecos << ':' << pent->pw_dir << ":/bin/sh" << endl;
}

string	dlreplace(const char *fname, string pw)
{
	string	user;
	string	ftl = fname;
	ftl += '-';
	ifstream fil(fname);
	ofstream tfile(ftl.c_str());
	string	saveline;
	while  (fil)  {
		string	s;
		getline(fil, s);
		size_t  cp = s.find_first_of(':');
		if  (cp != string::npos  &&  s.substr(0, cp-1) == "dl" && isdigit(s[cp-1]))  {
			saveline = s;
			break;
		}
		tfile << s << endl;
	}
	while  (fil)  {
		string	s;
		getline(fil, s);
		tfile << s << endl;
	}

	if  (saveline.length() > 0)  {
		size_t  cp = saveline.find_first_of(':');
		user = saveline.substr(0, cp);
		size_t  ep = saveline.find_first_of(':', cp+1);
		tfile << user << ':' << crypt_passwd((char *) pw.c_str()) << saveline.substr(ep) << endl;
	}
	else
		throw  error_report("No dl users found", 8);

	fil.close();
	tfile.close();
	ifstream  itfile(ftl.c_str());
	ofstream  ofil(fname);
	while  (itfile)  {
		string  s;
		getline(itfile, s);
		if  (s.length() > 0)
			ofil << s << endl;
	}
	unlink(ftl.c_str());
	return  user;
}

int	setup_socket(const char *portname)
{
	int  portnum;

	if  (isdigit(portname[0]))
		portnum = ntohs(atoi(portname));
	else  if  (servent  *sp = getservbyname(portname, "tcp"))
		portnum = sp->s_port;
	else  {
		ostringstream  ostr;
		ostr << "Unknown port name " << portname;
		throw  error_report(ostr.str(), 102);
	}

	int  sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if  (sockfd < 0)  {
		ostringstream  ostr;
		ostr << "Cannot create socket";
		throw  error_report(ostr.str(), 108);
		return  103;
	}

	sockaddr_in  sin;
	memset(&sin, '\0', sizeof(sin));
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_family = AF_INET;
	sin.sin_port = portnum;	// Already htons-ified

	if  (::bind(sockfd, (sockaddr *) &sin, sizeof(sin)) < 0)
		throw  error_report("Cannot bind socket", 104);

	if  (::listen(sockfd, SOMAXCONN) < 0)
		throw  error_report("Cannot listen socket", 105);

	return  sockfd;
}

void	pushout(int sockfd, const string &msg)
{
	const  char  *ob = msg.c_str();
	int	nbytes = msg.length();

	while  (nbytes > 0)  {
		int	nb = write(sockfd, ob, nbytes);
		if  (nb <= 0)
			return;
		nbytes -= nb;
		ob += nb;
	}
}

void	error_report::xml_report(int sockfd)
{
	xml_fd	xfd;
	xfd.initialise();
	xmlNodePtr  rt = xfd.start_memsave("ftperr");
	xfd.save_stringvalue(rt, "msg", msg.c_str());
	xfd.save_intvalue(rt, "code", code);
	string	outmsg;
	if  (xfd.finish_memsave(outmsg))
		pushout(sockfd, outmsg);
}

void	req_process(int sockfd, const char *dicname, const char *fname)
{
	string	inmsg;
	int	tocome;

	do  {
		char	inbuf[1025];
		int	nbytes = read(sockfd, inbuf, sizeof(inbuf)-1);
		if  (nbytes < 0)
			return;
		inbuf[nbytes-1] = '\0';
		inmsg += inbuf;
		tocome = 0;
		ioctl(sockfd, FIONREAD, &tocome);
	}  while  (tocome > 0);
	
	xml_fd  xfd;
	xfd.initialise();
	if  (!xfd.start_memload(inmsg))
		return;
	try  {
		xmlNodePtr  rt = xfd.get_root_element();
		string	username, password;
		bool  supp_user = xfd.load_stringvalue(rt, "user", username);
		bool  supp_pw = xfd.load_stringvalue(rt, "passwd", password);
		xfd.finish_load();
		if  (!supp_pw)
			password = generate_passwd(dicname);
		if  (supp_user)  {
			if  (findu(username, fname))
				replacepw(username, fname, password);
			else
				insertpw(username, fname, password);
		}
		else
			username = dlreplace(fname, password);

		//  Set up new message
		
		xfd.initialise();
		xmlNodePtr  ort = xfd.start_memsave("ftpreq");
		xfd.save_stringvalue(ort, "user", username.c_str());
		if  (!supp_pw)
			xfd.save_stringvalue(ort, "passwd", password.c_str());
		xfd.save_stringvalue(ort, "homed", gethomed(username, fname).c_str());
		string  outmsg;
		if  (!xfd.finish_memsave(outmsg))
			return;
		pushout(sockfd, outmsg);
	}
	catch  (error_report &e)  {
		e.xml_report(sockfd);
	}
}

int  main(int argc, char **argv)
{
	const	char	*fname = "/etc/ftppasswd";
	const	char	*dicname = "/usr/share/dict/words";
	string	user, passwd;
	bool	gen_passwd = false;
	bool	is_daemon = false;
	const   char	*portname = "2501";

	int	ch;

	srandom(time(0));

	while  ((ch = getopt(argc, argv, "f:d:u:p:DP:")) >= 0)
		switch  (ch)  {
		default:
			cerr << "Usage: " << argv[0] << " [-D [-P port]] [-f pw-file] [-d dict-file] [-u user] [-p passwd]" << endl;
			return  1;
		case  'f':
			fname = optarg;
			break;
		case  'd':
			dicname = optarg;
			break;
		case  'u':
			user = optarg;
			break;
		case  'p':
			passwd = optarg;
			break;
		case  'D':
			is_daemon = true;
			break;
		case  'P':
			portname = optarg;
			break;
		}

	if  (strcmp(fname, "/etc/passwd") == 0  || strcmp(fname, "/etc/shadow") == 0  || strcmp(fname, "/etc/group") == 0)  {
		cerr << "I refuse to set passwords in " << fname << endl;
		return  100;
	}

	struct  stat  sbuf;
	if  (lstat(fname, &sbuf) >= 0  &&  (sbuf.st_mode & S_IFMT) == S_IFLNK)  {
		cerr << "I refuse to set passwords in symbolic links such as " << fname << endl;
		return  101;
	}

	if  (is_daemon)  {

		if  (fork() > 0)
			exit(0);

		signal(SIGHUP, SIG_IGN);
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		
		int	sockfd;
		try  {
			sockfd = setup_socket(portname);
		}
		catch  (error_report &e)  {
			e.stderr_report();
			return  e.code;
		}
		for   (;;)  {
			sockaddr_in  cls;
			socklen_t    sinl = sizeof(cls);
			int	newsock;
			if  ((newsock = ::accept(sockfd, (sockaddr *) &cls, &sinl)) < 0)  {
				cerr << "Trouble accepting on socket" << endl;
				return  110;
			}
			req_process(newsock, dicname, fname);
			close(newsock);
		}
	}
	else  {
		try  {
			if  (passwd.length() == 0)  {
				passwd = generate_passwd(dicname);
				gen_passwd = true;
			}

			if  (user.length() != 0)  {
				if  (findu(user, fname))
					replacepw(user, fname, passwd);
				else
					insertpw(user, fname, passwd);
			}
			else
				user = dlreplace(fname, passwd);
		}
		catch  (error_report &e)  {
			e.stderr_report();
			return  e.code;
		}

		if  (gen_passwd)
			cout << user << ':' <<  passwd << endl;
	}
	return  0;
}
