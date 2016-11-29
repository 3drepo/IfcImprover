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

std::map < std::string, std::pair<IfcSchema::IfcRelAssociatesMaterial*, IfcSchema::IfcSurfaceStyle*> >
getRelMatMap(IfcParse::IfcFile &ifcfile)
{
	std::map < std::string, std::pair<IfcSchema::IfcRelAssociatesMaterial*, IfcSchema::IfcSurfaceStyle*> > matToIfcRelMat;
	auto materialEntities = ifcfile.entitiesByType("IfcMaterial");
	for (const auto &en : *materialEntities)
	{
		auto mat = dynamic_cast<const IfcSchema::IfcMaterial*>(en);
		if (mat)
		{
			auto ref = ifcfile.entitiesByReference(mat->entity->id());
			for (auto &r : *ref)
			{
				if (r->type() == IfcSchema::Type::Enum::IfcRelAssociatesMaterial)
				{
					matToIfcRelMat[mat->Name()] = { dynamic_cast<IfcSchema::IfcRelAssociatesMaterial*>(r), nullptr };
				}
			}
		}
	}

	auto surfaceItems = ifcfile.entitiesByType("IfcSurfaceStyle");
	for (auto &en : *surfaceItems)
	{
		auto mat = dynamic_cast<IfcSchema::IfcSurfaceStyle*>(en);
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

IfcSchema::IfcGeometricRepresentationItem* 
cloneGeoItem
(const IfcSchema::IfcGeometricRepresentationItem* org)
{
	IfcSchema::IfcGeometricRepresentationItem* res = nullptr;
	
	//FIXME: We have to do this as IFCOpenShell has no facility to clone an item...
	switch (org->type())
	{
	case IfcSchema::Type::Enum::IfcGeometricSet:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcGeometricSet*>(org);
		res = new IfcSchema::IfcGeometricSet(ptr->Elements());
	}
	break;
	case IfcSchema::Type::Enum::IfcHalfSpaceSolid:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcHalfSpaceSolid*>(org);
		res = new IfcSchema::IfcHalfSpaceSolid(ptr->BaseSurface(), ptr->AgreementFlag());
	}
	break;
	case IfcSchema::Type::Enum::IfcLightSource:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcLightSource*>(org);
		res = new IfcSchema::IfcLightSource(ptr->Name(), ptr->LightColour(), ptr->AmbientIntensity(), ptr->Intensity());
	}
	break;
	case IfcSchema::Type::Enum::IfcOneDirectionRepeatFactor:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcOneDirectionRepeatFactor*>(org);
		res = new IfcSchema::IfcOneDirectionRepeatFactor(ptr->RepeatFactor());
	}
	break;
	case IfcSchema::Type::Enum::IfcPlacement:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcPlacement*>(org);
		res = new IfcSchema::IfcPlacement(ptr->Location());
	}
	break;
	case IfcSchema::Type::Enum::IfcPlanarExtent:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcPlanarExtent*>(org);
		res = new IfcSchema::IfcPlanarExtent(ptr->SizeInX(), ptr->SizeInY());
	}
	break;
	case IfcSchema::Type::Enum::IfcPointOnCurve:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcPointOnCurve*>(org);
		res = new IfcSchema::IfcPointOnCurve(ptr->BasisCurve(), ptr->PointParameter());
	}
	break;
	case IfcSchema::Type::Enum::IfcPointOnSurface:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcPointOnSurface*>(org);
		res = new IfcSchema::IfcPointOnSurface(ptr->BasisSurface(), ptr->PointParameterU(), ptr->PointParameterV());
	}
	break;
	case IfcSchema::Type::Enum::IfcCartesianPoint:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcCartesianPoint*>(org);
		res = new IfcSchema::IfcCartesianPoint(ptr->Coordinates());
	}
	break;
	case IfcSchema::Type::Enum::IfcSectionedSpine:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcSectionedSpine*>(org);
		res = new IfcSchema::IfcSectionedSpine(ptr->SpineCurve(), ptr->CrossSections(), ptr->CrossSectionPositions());
	}
	break;
	case IfcSchema::Type::Enum::IfcShellBasedSurfaceModel:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcShellBasedSurfaceModel*>(org);
		res = new IfcSchema::IfcShellBasedSurfaceModel(ptr->SbsmBoundary());
	}
	break;
	case IfcSchema::Type::Enum::IfcSweptDiskSolid:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcSweptDiskSolid*>(org);
		res = new IfcSchema::IfcSweptDiskSolid(ptr->Directrix(), ptr->Radius(), ptr->InnerRadius(), ptr->StartParam(), ptr->EndParam());
	}
	break;
	case IfcSchema::Type::Enum::IfcSweptAreaSolid:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcSweptAreaSolid*>(org);
		res = new IfcSchema::IfcSweptAreaSolid(ptr->SweptArea(), ptr->Position());
	}
	break;
	case IfcSchema::Type::Enum::IfcCsgSolid:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcCsgSolid*>(org);
		res = new IfcSchema::IfcCsgSolid(ptr->TreeRootExpression());
	}
	break;
	case IfcSchema::Type::Enum::IfcManifoldSolidBrep:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcManifoldSolidBrep*>(org);
		res = new IfcSchema::IfcManifoldSolidBrep(ptr->Outer());
	}
	break;
	
	case IfcSchema::Type::Enum::IfcSweptSurface:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcSweptSurface*>(org);
		res = new IfcSchema::IfcSweptSurface(ptr->SweptCurve(), ptr->Position());
	}
	break;
	case IfcSchema::Type::Enum::IfcCurveBoundedPlane:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcCurveBoundedPlane*>(org);
		res = new IfcSchema::IfcCurveBoundedPlane(ptr->BasisSurface(), ptr->OuterBoundary(), ptr->InnerBoundaries());
	}
	break;

	case IfcSchema::Type::Enum::IfcRectangularTrimmedSurface:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcRectangularTrimmedSurface*>(org);
		res = new IfcSchema::IfcRectangularTrimmedSurface(ptr->BasisSurface(), ptr->U1(), ptr->V1(), ptr->U2(), ptr->V2(), ptr->Usense(), ptr->Vsense());
	}
	break;
	case IfcSchema::Type::Enum::IfcElementarySurface:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcElementarySurface*>(org);
		res = new IfcSchema::IfcElementarySurface(ptr->Position());
	}
	break;
	case IfcSchema::Type::Enum::IfcTextLiteral:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcTextLiteral*>(org);
		res = new IfcSchema::IfcTextLiteral(ptr->Literal(), ptr->Placement(), ptr->Path());
	}
	break;
	case IfcSchema::Type::Enum::IfcVector:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcVector*>(org);
		res = new IfcSchema::IfcVector(ptr->Orientation(), ptr->Magnitude());
	}
	break;
	case IfcSchema::Type::Enum::IfcAnnotationFillArea:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcAnnotationFillArea*>(org);
		res = new IfcSchema::IfcAnnotationFillArea(ptr->OuterBoundary(), ptr->InnerBoundaries());
	}
	break;
	case IfcSchema::Type::Enum::IfcAnnotationSurface:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcAnnotationSurface*>(org);
		res = new IfcSchema::IfcAnnotationSurface(ptr->Item(), ptr->TextureCoordinates());
	}
	break;
	case IfcSchema::Type::Enum::IfcBooleanResult:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcBooleanResult*>(org);
		res = new IfcSchema::IfcBooleanResult(ptr->Operator(), ptr->FirstOperand(), ptr->SecondOperand());
	}
	break;
	case IfcSchema::Type::Enum::IfcBoundingBox:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcBoundingBox*>(org);
		res = new IfcSchema::IfcBoundingBox(ptr->Corner(), ptr->XDim(), ptr->YDim(), ptr->ZDim());
	}
	break;
	case IfcSchema::Type::Enum::IfcCartesianTransformationOperator:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcCartesianTransformationOperator*>(org);
		res = new IfcSchema::IfcCartesianTransformationOperator(ptr->Axis1(), ptr->Axis2(), ptr->LocalOrigin(), ptr->Scale());
	}
	break;
	case IfcSchema::Type::Enum::IfcCompositeCurveSegment:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcCompositeCurveSegment*>(org);
		res = new IfcSchema::IfcCompositeCurveSegment(ptr->Transition(), ptr->SameSense(), ptr->ParentCurve());
	}
	break;
	case IfcSchema::Type::Enum::IfcCsgPrimitive3D:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcCsgPrimitive3D*>(org);
		res = new IfcSchema::IfcCsgPrimitive3D(ptr->Position());
	}
	break;
	case IfcSchema::Type::Enum::IfcLine:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcLine*>(org);
		res = new IfcSchema::IfcLine(ptr->Pnt(), ptr->Dir());
	}
	break;
	case IfcSchema::Type::Enum::IfcOffsetCurve2D:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcOffsetCurve2D*>(org);
		res = new IfcSchema::IfcOffsetCurve2D(ptr->BasisCurve(), ptr->Distance(), ptr->SelfIntersect());
	}
	break;
	case IfcSchema::Type::Enum::IfcOffsetCurve3D:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcOffsetCurve3D*>(org);
		res = new IfcSchema::IfcOffsetCurve3D(ptr->BasisCurve(), ptr->Distance(), ptr->SelfIntersect(), ptr->RefDirection());
	}
	break;
	case IfcSchema::Type::Enum::IfcCompositeCurve:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcCompositeCurve*>(org);
		res = new IfcSchema::IfcCompositeCurve(ptr->Segments(), ptr->SelfIntersect());
	}
	break;
	case IfcSchema::Type::Enum::IfcPolyline:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcPolyline*>(org);
		res = new IfcSchema::IfcPolyline(ptr->Points());
	}
	break;
	case IfcSchema::Type::Enum::IfcTrimmedCurve:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcTrimmedCurve*>(org);
		res = new IfcSchema::IfcTrimmedCurve(ptr->BasisCurve(), ptr->Trim1(), ptr->Trim2(), ptr->SenseAgreement(), ptr->MasterRepresentation());
	}
	break;
	case IfcSchema::Type::Enum::IfcBSplineCurve:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcBSplineCurve*>(org);
		res = new IfcSchema::IfcBSplineCurve(ptr->Degree(), ptr->ControlPointsList(), ptr->CurveForm(), ptr->ClosedCurve(), ptr->SelfIntersect());
	}
	break;
	case IfcSchema::Type::Enum::IfcDefinedSymbol:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcDefinedSymbol*>(org);
		res = new IfcSchema::IfcDefinedSymbol(ptr->Definition(), ptr->Target());
	}
	break;
	case IfcSchema::Type::Enum::IfcConic:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcConic*>(org);
		res = new IfcSchema::IfcConic(ptr->Position());
	}
	break;
	case IfcSchema::Type::Enum::IfcDirection:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcDirection*>(org);
		res = new IfcSchema::IfcDirection(ptr->DirectionRatios());
	}
	break;
	case IfcSchema::Type::Enum::IfcDraughtingCallout:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcDraughtingCallout*>(org);
		res = new IfcSchema::IfcDraughtingCallout(ptr->Contents());
	}
	break;
	case IfcSchema::Type::Enum::IfcFaceBasedSurfaceModel:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcFaceBasedSurfaceModel*>(org);
		res = new IfcSchema::IfcFaceBasedSurfaceModel(ptr->FbsmFaces());
	}
	break;
	case IfcSchema::Type::Enum::IfcFillAreaStyleHatching:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcFillAreaStyleHatching*>(org);
		res = new IfcSchema::IfcFillAreaStyleHatching(ptr->HatchLineAppearance(), ptr-> StartOfNextHatchLine(), ptr->PointOfReferenceHatchLine(), ptr->PatternStart(), ptr->HatchLineAngle());
	}
	break;
	case IfcSchema::Type::Enum::IfcFillAreaStyleTileSymbolWithStyle:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcFillAreaStyleTileSymbolWithStyle*>(org);
		res = new IfcSchema::IfcFillAreaStyleTileSymbolWithStyle(ptr->Symbol());
	}
	break;
	case IfcSchema::Type::Enum::IfcFillAreaStyleTiles:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcFillAreaStyleTiles*>(org);
		res = new IfcSchema::IfcFillAreaStyleTiles(ptr->TilingPattern(), ptr->Tiles(), ptr->TilingScale());
	}
	break;
	case IfcSchema::Type::Enum::IfcFacetedBrep:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcFacetedBrep*>(org);
		res = new IfcSchema::IfcFacetedBrep(ptr->Outer());
	}
	break;
	case IfcSchema::Type::Enum::IfcFacetedBrepWithVoids:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcFacetedBrepWithVoids*>(org);
		res = new IfcSchema::IfcFacetedBrepWithVoids(ptr->Outer(), ptr->Voids());
	}
	break;
	case IfcSchema::Type::Enum::IfcExtrudedAreaSolid:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcExtrudedAreaSolid*>(org);
		res = new IfcSchema::IfcExtrudedAreaSolid(ptr->SweptArea(), ptr->Position(), ptr-> ExtrudedDirection(), ptr->Depth());
	}
	break;
	case IfcSchema::Type::Enum::IfcRevolvedAreaSolid:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcRevolvedAreaSolid*>(org);
		res = new IfcSchema::IfcRevolvedAreaSolid(ptr->SweptArea(), ptr->Position(), ptr->Axis(), ptr->Angle());
	}
	break;
	case IfcSchema::Type::Enum::IfcSurfaceCurveSweptAreaSolid:
	{
		auto ptr = dynamic_cast<const IfcSchema::IfcSurfaceCurveSweptAreaSolid*>(org);
		res = new IfcSchema::IfcSurfaceCurveSweptAreaSolid(ptr->SweptArea(), ptr->Position(), ptr->Directrix(), ptr->StartParam(), ptr->EndParam(), ptr->ReferenceSurface());
	}
	break;
	default:
		std::cerr << "Unrecognised type for this entity:  " << org->entity->toString(); exit(-1);
	}


	return res;
}

std::set<IfcSchema::IfcGeometricRepresentationItem*>
extractGeoRepItems(IfcSchema::IfcRepresentationItem* repItem,
std::set<IfcSchema::IfcRepresentationItem*> &geoItems,
std::set<IfcSchema::IfcRepresentation*> &geoReps,
std::set<IfcSchema::IfcRepresentationMap*> &seenMaps,
IfcParse::IfcFile &ifcFile,
IfcSchema::IfcRepresentationItem* &newItem
)
{
	std::set<IfcSchema::IfcGeometricRepresentationItem*> items;
	if (auto geoItem = dynamic_cast<IfcSchema::IfcGeometricRepresentationItem*>(repItem))
	{
		if (geoItems.find(geoItem) != geoItems.end())
		{
			//This item already exists, clone it to avoid material clashing
			newItem = cloneGeoItem(geoItem);
			ifcFile.addEntity(newItem);
			
			if (geoItem->entity->id() == 245374 || geoItem->entity->id() == 245393 || geoItem->entity->id() == 245412)
			{
				std::cout << "Cloned item has id of " << newItem->entity->id() << " from " << geoItem->entity->id() << std::endl;
			}

			geoItem = (IfcSchema::IfcGeometricRepresentationItem*)newItem;

		}
		items.insert(geoItem);
	}
	else
	{
		//This must be a mapped item?
		auto mappedItem = dynamic_cast<IfcSchema::IfcMappedItem*>(repItem);
		//ensure we haven't seen this mapped Item before
		if (geoItems.find(mappedItem) != geoItems.end())
		{
			//We've seen this mapped item before. clone it to avoid material clashing
			mappedItem = new IfcSchema::IfcMappedItem(mappedItem->MappingSource(), mappedItem->MappingTarget());
			ifcFile.addEntity(mappedItem);
			newItem = mappedItem;

		}
		
		geoItems.insert(mappedItem);
		
		//ensure we haven't seen this representation map before
		auto repMap = mappedItem->MappingSource();
		if (seenMaps.find(repMap) != seenMaps.end())
		{
			//We've seen this mapped item before. clone it to avoid material clashing
			repMap = new IfcSchema::IfcRepresentationMap(repMap->MappingOrigin(), repMap->MappedRepresentation());
			ifcFile.addEntity(repMap);
			mappedItem->setMappingSource(repMap);
		
		}
		
		seenMaps.insert(repMap);	

		auto rep = mappedItem->MappingSource()->MappedRepresentation();
		if (geoReps.find(rep) != geoReps.end())
		{
			rep = new IfcSchema::IfcRepresentation(rep->ContextOfItems(), rep->RepresentationIdentifier(), rep->RepresentationType(), rep->Items());
			mappedItem->MappingSource()->setMappedRepresentation(rep);
			ifcFile.addEntity(rep);
		}
		geoReps.insert(rep);

		if (rep->Items()->size())
		{
			auto size = rep->Items()->size();
			auto itemsr = rep->Items();
			
			std::vector<std::pair<IfcSchema::IfcRepresentationItem *, IfcSchema::IfcRepresentationItem *>> changedChildren;
			for (auto &subRepItem : *itemsr)
			{
				IfcSchema::IfcRepresentationItem * newPtr = nullptr;
				auto childRes = extractGeoRepItems(subRepItem, geoItems, geoReps, seenMaps, ifcFile, newPtr);
				items.insert(childRes.begin(), childRes.end());
				
				if (newPtr)
				{
					changedChildren.push_back({ subRepItem, newPtr });
				}
			}

			//New instances of these children were created, reflect them on the mapped item
			for (auto &ptr : changedChildren)
			{
				itemsr->remove(ptr.first);
				itemsr->push(ptr.second);
			}
			rep->setItems(itemsr);
		}
	}

	return items;
}

std::set<IfcSchema::IfcGeometricRepresentationItem*> 
findGeoRepItems(const IfcSchema::IfcProduct *relProd,
std::set<IfcSchema::IfcRepresentationItem*> &geoItems,
std::set<IfcSchema::IfcRepresentation*> &geoReps,
std::set<IfcSchema::IfcRepresentationMap*> &seenMaps,
	IfcParse::IfcFile &ifcFile)
{
	std::set<IfcSchema::IfcGeometricRepresentationItem*> res;
	auto shapRep = dynamic_cast<const IfcSchema::IfcProductRepresentation*>(relProd->Representation());
	if (shapRep)
	{
		auto reps = shapRep->Representations();

		for (const auto rep : *reps)
		{
			auto shape = dynamic_cast<const IfcSchema::IfcShapeRepresentation*>(rep);

			auto items = shape->Items();
			std::vector<std::pair<IfcSchema::IfcRepresentationItem *, IfcSchema::IfcRepresentationItem *>> changedChildren;
			for (auto item : *items)
			{
				IfcSchema::IfcRepresentationItem * newPtr = nullptr;
				auto geoRepItems = extractGeoRepItems(item, geoItems, geoReps, seenMaps, ifcFile, newPtr);
				if (newPtr)
				{
					changedChildren.push_back({ item, newPtr });
				}
				res.insert(geoRepItems.begin(), geoRepItems.end());
			}

			//New instances of these children were created, reflect them on the mapped item
			for (auto &ptr : changedChildren)
			{
				items->remove(ptr.first);
				items->push(ptr.second);
			}
			rep->setItems(items);
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

	std::map<IfcSchema::IfcRepresentationItem*, IfcSchema::IfcStyledItem*> geoRepToStyle;

	auto styledItem = ifcfile.entitiesByType("IfcStyledItem");
	for (const auto &style : *styledItem)
	{
		auto s = dynamic_cast<IfcSchema::IfcStyledItem*>(style);
		if (s->hasItem())
			geoRepToStyle[s->Item()] = s;
	}

	std::map < std::string, std::pair<IfcSchema::IfcRelAssociatesMaterial*, IfcSchema::IfcSurfaceStyle*>> matToIfcRelMat = getRelMatMap(ifcfile);
	std::set<IfcSchema::IfcRepresentationItem*> geoList;
	std::set<IfcSchema::IfcRepresentationMap*> seenMaps;
	std::set<IfcSchema::IfcRepresentation*> geoReps;

	auto metadataEntities = ifcfile.entitiesByType("IfcPropertySingleValue");
	IfcEntityList::ptr newEntities(new IfcEntityList());
	for (const auto &meta : *metadataEntities)
	{
		auto singleProp = dynamic_cast<const IfcSchema::IfcPropertySingleValue*>(meta);
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


					if (matIt != matToIfcRelMat.end())
					{
						IfcTemplatedEntityList< IfcSchema::IfcRoot >::ptr relatingObjects;
						std::set<IfcSchema::IfcRoot*> objs;
						if (matIt->second.first)
						{
							relatingObjects = matIt->second.first->RelatedObjects();
							 objs.insert(relatingObjects->begin(), relatingObjects->end());
						}

						std::set<IfcSchema::IfcGeometricRepresentationItem*> geoItems;
						
						auto refs = ifcfile.entitiesByReference(singleProp->entity->id());
						for (const auto & r : *refs)
						{
							auto refs2 = ifcfile.entitiesByReference(r->entity->id());
							for (const auto & r2 : *refs2)
							{
								if (r2->type() == IfcSchema::Type::Enum::IfcRelDefinesByProperties)
								{
									auto relProp = dynamic_cast<const IfcSchema::IfcRelDefinesByProperties*>(r2);
									auto relObjs = relProp->RelatedObjects();
									for (const auto & r3 : *relObjs)
									{
										auto relProd = dynamic_cast<const IfcSchema::IfcProduct*>(r3);
										std::set<IfcSchema::IfcGeometricRepresentationItem*> pGeoItems = findGeoRepItems(relProd, geoList, geoReps, seenMaps, ifcfile);
										geoItems.insert(pGeoItems.begin(), pGeoItems.end());
										geoList.insert(pGeoItems.begin(), pGeoItems.end());
										/*if (debug && r3->entity->id() == 245890)
										{*/
											//std::cout << "found the entity" << std::endl;
											//std::cout << "Geo items are: " << std::endl;
											for (const auto g : pGeoItems)
											{
												if (g->entity->id() == 245374 || g->entity->id() == 245393 || g->entity->id() == 245412)
												{
													std::cout << "found geo entity in " << relProd->entity->id() << std::endl;
												}
												//std::cout << "\t" << g->entity->id() << std::endl; 
											}
										//}

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
								if (geoItem->entity->id() == 245374 || geoItem->entity->id() == 245393 || geoItem->entity->id() == 245412)
								{
									std::cout << "[" << value<<"]Setting material for " << geoItem->entity->id() << " with " << matIt->first << std::endl;
								}
								if (geoRepToStyle.find(geoItem) != geoRepToStyle.end())
								{
									geoRepToStyle[geoItem]->setItem(nullptr);
									//It already has a surface item. does that mean it already has a material?
								}
								
								//No surface style
								IfcTemplatedEntityList< IfcSchema::IfcPresentationStyleAssignment >::ptr list(new IfcTemplatedEntityList< IfcSchema::IfcPresentationStyleAssignment >);

								auto styleAssignment = new IfcSchema::IfcPresentationStyleAssignment(surfaceList);
								list->push(styleAssignment);
								IfcSchema::IfcStyledItem* newStyleItem = new IfcSchema::IfcStyledItem(geoItem, list, std::string());
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