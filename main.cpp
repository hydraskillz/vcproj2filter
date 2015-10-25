/*
 * Author      : Matthew Johnson
 * Description : Generates the .filter file from a .vcproj file.
 * Usage       : vcproj2filter 'path/to/my.vcproj'
 */
#include "tinyxml2/tinyxml2.h"
#include <vector>
#include <map>
#include <set>

using namespace tinyxml2;

struct FileEntry
{
	std::string m_filename;
	std::string m_filtername;
};

std::set<std::string> g_filters;
std::map<std::string, std::vector<FileEntry>> g_files;

void ParseGroup(XMLElement* itemGroup, const char* group)
{
	std::vector<FileEntry>& list = g_files[group];
	while (itemGroup)
	{
		const char* cpath = itemGroup->Attribute("Include");
		if (cpath)
		{
			FileEntry entry = {cpath, ""};
			
			do
			{
				while (*cpath == '.') ++cpath;
				if (*cpath == '\\') ++cpath;
			} while (*cpath == '.');

			std::string path(cpath);
			int lastSlash = path.find_last_of('\\');

			if (lastSlash != -1)
			{
				path.assign(cpath, cpath + lastSlash);
				g_filters.insert(path);
				entry.m_filtername = path;
			}

			list.push_back(entry);
		}
		itemGroup = itemGroup->NextSiblingElement(group);
	}
}

void ParseItemGroup(XMLElement* itemGroup)
{
	if (itemGroup->Attribute("Label"))
	{
		return;
	}

	XMLElement* cppElem = itemGroup->FirstChildElement("ClCompile");
	ParseGroup(cppElem, "ClCompile");

	XMLElement* hElem = itemGroup->FirstChildElement("ClInclude");
	ParseGroup(hElem, "ClInclude");

	XMLElement* nElem = itemGroup->FirstChildElement("None");
	ParseGroup(nElem, "None");
}

void MakeFilterFile(const char* filename)
{
	FILE* file = fopen(filename, "w");
	if (file == nullptr)
		return;

	XMLPrinter printer(file, false);

	printer.PushHeader(false, true);

	printer.OpenElement("Project");
	printer.PushAttribute("ToolsVersion", "4.0");
	printer.PushAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

	printer.OpenElement("ItemGroup");
	
	for (auto itr = g_filters.begin(); itr != g_filters.end(); ++itr)
	{
		printer.OpenElement("Filter");
		printer.PushAttribute("Include", (*itr).c_str());
		printer.CloseElement();
	}

	printer.CloseElement();

	printer.OpenElement("ItemGroup");

	for (auto itr = g_files.begin(); itr != g_files.end(); ++itr)
	{
		const char* itemType = itr->first.c_str();
		const std::vector<FileEntry>& files = itr->second;
		for (auto jtr = files.begin(); jtr != files.end(); ++jtr)
		{
			printer.OpenElement(itemType);
			printer.PushAttribute("Include", jtr->m_filename.c_str());
			if (!jtr->m_filtername.empty())
			{
				printer.OpenElement("Filter");
				printer.PushText(jtr->m_filtername.c_str());
				printer.CloseElement();
			}
			printer.CloseElement();
		}
	}

	printer.CloseElement();

	printer.CloseElement();
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Usage: vcproj2filter 'path/to/my.vcproj'\n");
		return 1;
	}

	const char* vcproj = argv[1];

	XMLDocument doc;
	XMLError res = doc.LoadFile(vcproj);

	if (res != XML_SUCCESS)
	{
		printf("Error: Could not open '%s'\n", vcproj);
		return 1;
	}

	XMLNode* root = doc.FirstChild();
	XMLElement* projectGroup = root->NextSiblingElement("Project");
	XMLElement* itemGroup = projectGroup->FirstChildElement("ItemGroup");
	while (itemGroup)
	{
		ParseItemGroup(itemGroup);
		itemGroup = itemGroup->NextSiblingElement("ItemGroup");
	}

	std::string output = vcproj;
	output += ".filters";

	MakeFilterFile(output.c_str());

	return 0;
}