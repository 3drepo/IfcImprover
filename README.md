IFC Improver by 3D Repo
=========
IFC Improver is a tool that aims to improve an IFC File by means of optimising the file, or enhancing the file by various means.

Currently it has the following functionality:
* [Material Override](#material-override)

## Material Override
Materials assigned to geometries within the IFC file can be modified, with the geometries being reassigning it's material to a different existing material within the IFC. This requires a CSV file depicting the relationship between Materials and Metadata properties. The program will then find all geometries associated with this metadata and reassign their materials.

This is mainly targeted at imperfect exports created from Revit due to unconventional material settings on geometries.

To execute this, run the program as follows:
`IfcImprover.exe <input IFC file> <output IFC file> <CSV file>

### CSV file format
The CSV file is expected to be as follows:

|  Material Name |  Metadata Field Name | Matching Metadata Value 1  | Matching Metadata Value 2  | ...  |
| --- | --- | --- | --- | --- |
| Carbon Steel | System Code | AB | BC | .. |

Under this example, any geometries associated with the System code `AB` or `BC` will be assigned to an existing material named Carbon Steel
