/**
*  Copyright (C) 2016 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ifcparse/IfcParse.h>
#include <ifcparse/IfcFile.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

std::map<std::string, std::map<std::string, std::string>> processCSVFile(const std::string &csvFile)
{
	std::ifstream fs(csvFile);
	std::string line;
	std::map<std::string, std::map<std::string, std::string>> dataToMat;
	if (std::getline(fs, line))
	{
		//skipping the first line which should be headers
		while (std::getline(fs, line))
		{
			std::stringstream ss(line);
			std::string item, matName, metaFieldName;
			std::vector<std::string> metaFieldData;
			int count = 0;
			while (std::getline(ss, item, ','))
			{
				//1 is material name, 2 is field to match, 3 onwards are the matching names
				switch (count++)
				{
				case 0:
					//Material name
					matName = item;
					break;
				case 1:
					metaFieldName = item;
					if (dataToMat.find(metaFieldName) == dataToMat.end())
					{
						dataToMat[metaFieldName] = std::map<std::string, std::string>();
					}
					break;
				default:
					if (!item.empty())
					{
						if (dataToMat[metaFieldName].find(item) == dataToMat[metaFieldName].end())
							dataToMat[metaFieldName][item] = matName;
						else
							std::cerr << "Found value mapped to multiple material: " << item << std::endl;
					}
				}
			}
		}
	}

	return dataToMat;
}

std::map < std::string, std::pair<Ifc2x3::IfcRelAssociatesMaterial*, Ifc2x3::IfcSurfaceStyle*> >
getRelMatMap(IfcParse::IfcFile &ifcfile)
{
	std::map < std::string, std::pair<Ifc2x3::IfcRelAssociatesMaterial*, Ifc2x3::IfcSurfaceStyle*> > matToIfcRelMat;
	auto materialEntities = ifcfile.entitiesByType("IfcMaterial");
	for (const auto &en : *materialEntities)
	{
		auto mat = dynamic_cast<const Ifc2x3::IfcMaterial*>(en);
		if (mat)
		{
			auto ref = ifcfile.entitiesByReference(mat->entity->id());
			for (auto &r : *ref)
			{
				if (r->type() == Ifc2x3::Type::Enum::IfcRelAssociatesMaterial)
				{
					matToIfcRelMat[mat->Name()] = { dynamic_cast<Ifc2x3::IfcRelAssociatesMaterial*>(r), nullptr };
				}
			}
		}
	}

	auto surfaceItems = ifcfile.entitiesByType("IfcSurfaceStyle");
	for (auto &en : *surfaceItems)
	{
		auto mat = dynamic_cast<Ifc2x3::IfcSurfaceStyle*>(en);
		if (mat)
		{
			if (matToIfcRelMat.find(mat->Name()) != matToIfcRelMat.end())
				matToIfcRelMat[mat->Name()].second = mat;
			else
			{
				//Should already have an entry.
				std::cerr << "Failed to find entry for " << mat->Name() << std::endl;
				matToIfcRelMat[mat->Name()] = { nullptr, mat};
			}
		}
	}

	return matToIfcRelMat;
}

std::set<Ifc2x3::IfcGeometricRepresentationItem*>
extractGeoRepItems(Ifc2x3::IfcRepresentationItem* repItem
)
{
	std::set<Ifc2x3::IfcGeometricRepresentationItem*> items;
	if (auto geoItem = dynamic_cast<Ifc2x3::IfcGeometricRepresentationItem*>(repItem))
	{
		items.insert(geoItem);
	}
	else
	{
		//This must be a mapped item?
		auto mappedItem = dynamic_cast<Ifc2x3::IfcMappedItem*>(repItem);
		auto rep = mappedItem->MappingSource()->MappedRepresentation();
		if (rep->Items()->size())
		{
			auto size = rep->Items()->size();
			auto itemsr = rep->Items();
			for (auto &subRepItem : *itemsr)
			{
				auto childRes = extractGeoRepItems(subRepItem);
				items.insert(childRes.begin(), childRes.end());
			}
		}
	}

	return items;
}

std::set<Ifc2x3::IfcGeometricRepresentationItem*> 
findGeoRepItems(const Ifc2x3::IfcProduct *relProd)
{
	std::set<Ifc2x3::IfcGeometricRepresentationItem*> res;
	auto shapRep = dynamic_cast<const Ifc2x3::IfcProductRepresentation*>(relProd->Representation());
	if (shapRep)
	{
		auto reps = shapRep->Representations();

		for (const auto rep : *reps)
		{
			auto shape = dynamic_cast<const Ifc2x3::IfcShapeRepresentation*>(rep);

			auto items = shape->Items();
			for (auto item : *items)
			{
				auto geoRepItems = extractGeoRepItems(item);
				res.insert(geoRepItems.begin(), geoRepItems.end());
			}
		}
	}

	return res;
}

void updateFile(const std::string &inputFile, std::string &outputfile,
	const std::map<std::string, std::map<std::string, std::string>> &matMap)
{
	IfcParse::IfcFile ifcfile;
	if (!ifcfile.Init(inputFile))
	{
		std::cerr << "Failed initialising " << inputFile << std::endl;
		return;
	}

	std::map<Ifc2x3::IfcRepresentationItem*, Ifc2x3::IfcStyledItem*> geoRepToStyle;

	auto styledItem = ifcfile.entitiesByType("IfcStyledItem");
	for (const auto &style : *styledItem)
	{
		auto s = dynamic_cast<Ifc2x3::IfcStyledItem*>(style);
		if (s->hasItem())
			geoRepToStyle[s->Item()] = s;
	}


	std::ofstream is("C:\\Users\\Carmen\\Desktop\\in.ifc");

	is << ifcfile;

	std::map < std::string, std::pair<Ifc2x3::IfcRelAssociatesMaterial*, Ifc2x3::IfcSurfaceStyle*>> matToIfcRelMat = getRelMatMap(ifcfile);

	auto metadataEntities = ifcfile.entitiesByType("IfcPropertySingleValue");
	IfcEntityList::ptr newEntities(new IfcEntityList());
	for (const auto &meta : *metadataEntities)
	{
		auto singleProp = dynamic_cast<const Ifc2x3::IfcPropertySingleValue*>(meta);
		if (singleProp->hasNominalValue())
		{
			std::string fieldName = singleProp->Name();
			std::string value = singleProp->NominalValue()->valueAsString();
			bool debug = value == "(FAI)";
			auto it = matMap.find(fieldName);
			if (it != matMap.end())
			{
				auto &valueMap = it->second;
				auto valueIt = valueMap.find(value);
				if (valueIt != valueMap.end())
				{
					const auto matName = valueIt->second;
					auto matIt = matToIfcRelMat.find(matName);
					if (debug)
					{
						std::cout << "Material for FAI: " << matIt->second.second->entity->toString() << std::endl;
					}

					if (matIt != matToIfcRelMat.end())
					{
						IfcTemplatedEntityList< Ifc2x3::IfcRoot >::ptr relatingObjects;
						std::set<Ifc2x3::IfcRoot*> objs;
						if (matIt->second.first)
						{
							relatingObjects = matIt->second.first->RelatedObjects();
							 objs.insert(relatingObjects->begin(), relatingObjects->end());
						}

						std::set<Ifc2x3::IfcGeometricRepresentationItem*> geoItems;
						
						auto refs = ifcfile.entitiesByReference(singleProp->entity->id());
						for (const auto & r : *refs)
						{
							auto refs2 = ifcfile.entitiesByReference(r->entity->id());
							for (const auto & r2 : *refs2)
							{
								if (r2->type() == Ifc2x3::Type::Enum::IfcRelDefinesByProperties)
								{
									auto relProp = dynamic_cast<const Ifc2x3::IfcRelDefinesByProperties*>(r2);
									auto relObjs = relProp->RelatedObjects();
									for (const auto & r3 : *relObjs)
									{
										auto relProd = dynamic_cast<const Ifc2x3::IfcProduct*>(r3);
										std::set<Ifc2x3::IfcGeometricRepresentationItem*> pGeoItems = findGeoRepItems(relProd);
										geoItems.insert(pGeoItems.begin(), pGeoItems.end());
																				
										if (objs.find(r3) == objs.end())
										{
											relatingObjects->push(r3);
										}
									}
								}
								else
								{
									std::cerr << "Unexpected type : " << r2->type() << std::endl;
									std::cout << r2->entity->toString() << std::endl;
								}
							}
						}

						//Add objects to Material link
						if (matIt->second.first)
							matIt->second.first->setRelatedObjects(relatingObjects);
						//Create surface items that references the geo items
						if (matIt->second.second)
						{
							auto surfaceStyle = matIt->second.second;
							IfcEntityList::ptr surfaceList(new IfcEntityList);
							surfaceList->push(surfaceStyle);
							for (auto &geoItem : geoItems)
							{
								if (geoRepToStyle.find(geoItem) != geoRepToStyle.end())
								{
									geoRepToStyle[geoItem]->setItem(nullptr);
									//It already has a surface item. does that mean it already has a material?
								}
								
								//No surface style
								IfcTemplatedEntityList< Ifc2x3::IfcPresentationStyleAssignment >::ptr list(new IfcTemplatedEntityList< Ifc2x3::IfcPresentationStyleAssignment >);

								auto styleAssignment = new Ifc2x3::IfcPresentationStyleAssignment(surfaceList);
								list->push(styleAssignment);
								Ifc2x3::IfcStyledItem* newStyleItem = new Ifc2x3::IfcStyledItem(geoItem, list, std::string());
								newEntities->push(newStyleItem);
								
							}

						}
						
					}
					else
					{
						std::cerr << "Failed to find material " << matName << std::endl;
					}
				}
			}
		}
		else
		{
			std::cout << "no nominal value: " << meta->entity->toString() << std::endl;
		}
	}
	ifcfile.addEntities(newEntities);
	std::ofstream os(outputfile);

	os << ifcfile;
}

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		std::cerr << "Usage: " << argv[0] << " <input file> <output file> <csv file>" << std::endl;
		return EXIT_FAILURE;
	}

	std::string inputFile = argv[1];
	std::string outputFile = argv[2];
	std::string csvFile = argv[3];

	auto matMap = processCSVFile(csvFile);

	if (matMap.size())
	{
		for (const auto &item : matMap)
		{
			std::cout << "Mat Item: " << item.first << std::endl;
			for (const auto &pair : item.second)
			{
				std::cout << "\t" << pair.first << " : " << pair.second << std::endl;
			}
		}
		updateFile(inputFile, outputFile, matMap);
	}

	return EXIT_SUCCESS;
}