import shapely as sh
from pathlib import Path
import os.path


def create_wkt_file(directory_path, z_height, *polygons):
    file_path = directory_path + '\\' + z_height + '_' + os.path.basename(directory_path) + '.wkt'
    wkt_file = open(file_path, "w")  # ToDo was soll passieren wenn file schon existiert, aktuell überschreiben
    wkt_file.write("POLYGON ((0 0, 0 20000, 20000 20000, 20000 0, 0 0))\n")  # ToDo an Bounding Box anpassen

    for p in polygons:
        wkt_file.write(sh.to_wkt(p, 0))
        wkt_file.write("\n")

    wkt_file.close()


point1 = sh.Point(0, 10000)
point2 = sh.Point(10000, 10000)
point3 = sh.Point(10000, 0000)

point4 = sh.Point(0, 0)
point5 = sh.Point(10000, 10000)

p_lst = [point1, point2, point3]
p_lst2 = [point4, point5]
poly = sh.Polygon(p_lst)
poly2 = sh.LineString(p_lst2)


pattern_wkt = "shapely_test"
tiles_path = 'C:Users\Marie\AppData\Roaming\cura\\5.7\plugins\CuraEngineLayeredInfill\CuraEngineLayeredInfill\\tiles'  # ToDo flexibeler pfad
dir_path = tiles_path + '\\' + pattern_wkt
file_path = tiles_path + '\\' + pattern_wkt + '.wkt'

print(dir_path)

if not os.path.exists(dir_path):
    os.makedirs(dir_path)

if not os.path.exists(file_path):  # ToDo Fehlermeldung bei Nutzung Default file
    open(file_path, "x")

create_wkt_file(dir_path, "200", poly, poly2)
