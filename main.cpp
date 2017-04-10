// Jong Hoon Lee
// Newcastle University
// j94117@gmail.com
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#define DATA_LOCATION "./data/"

using namespace std;

template <typename T>
struct Vector2 {
	T uv[2];

	Vector2() {}

	Vector2(T u, T v) {
		uv[0] = u;
		uv[1] = v;
	}
};

template <typename T>
struct Vector3 {
	T xyz[3];

	Vector3() {}

	Vector3(T x, T y, T z) {
		xyz[0] = x;
		xyz[1] = y;
		xyz[2] = z;
	}
};

// I understand that this is redundant but I wanted to separate Face index from Vector3
struct Face {
	int indices[3];

	Face(int a, int b, int c) {
		indices[0] = a;
		indices[1] = b;
		indices[2] = c;
	}
};

struct Material {
	string name;
	Vector3<double> diffuse;
	Vector3<double> ambient;
	Vector3<double> specular;
	double opacity;
};

template <typename T>
struct Mesh {
	string name;
	vector<Vector3<T>> vertices;
	vector<Vector2<T>> texCoords;
	vector<Vector3<T>> normals; // with duplicates
	vector<Vector3<T>> normals_ndp; // without duplciates
	vector<Face> faces;
	Material mt;
	bool hasMtl = false;
};

// Extracting just the wanted numbers/strings
template <typename T>
void splitLine(const string& str, string delim, vector<T> &out) {
	std::size_t prev_pos = 0, pos;
	string f;

	while ((pos = str.find_first_of(delim, prev_pos)) != std::string::npos)
	{
		if (pos > prev_pos) {
			f = str.substr(prev_pos, pos - prev_pos);
			out.push_back(f);
		}
		prev_pos = pos + 1;
	}
	// Last number
	if (prev_pos < str.length()) {
		f = str.substr(prev_pos, std::string::npos);
		out.push_back(f);
	}
}

// Double type split template
template <>
void splitLine<double>(const string& str, string delim, vector<double> &out) {
	std::size_t prev_pos = 0, pos;
	string f;

	while ((pos = str.find_first_of(delim, prev_pos)) != std::string::npos)
	{
		if (pos > prev_pos) {
			f = str.substr(prev_pos, pos - prev_pos);
			out.push_back(atof(f.c_str()));
		}
		prev_pos = pos + 1;
	}
	// Last number
	if (prev_pos < str.length()) {
		f = str.substr(prev_pos, std::string::npos);
		out.push_back(atof(f.c_str()));
	}
}

// Int type split template
template <>
void splitLine<int>(const string& str, string delim, vector<int> &out) {
	std::size_t prev_pos = 0, pos;
	string f;
	int value;

	while ((pos = str.find_first_of(delim, prev_pos)) != std::string::npos)
	{
		if (pos > prev_pos) {
			f = str.substr(prev_pos, pos - prev_pos);
			value = stoi(f.c_str());
			out.push_back(value);
		}
		prev_pos = pos + 1;
	}
	// Last number
	if (prev_pos < str.length()) {
		f = str.substr(prev_pos, std::string::npos);
		value = stoi(f.c_str());
		out.push_back(value);
	}
}

// Compare function for Vector3 struct
template <typename T>
bool compare(Vector3<T> a, Vector3<T> b) {
	return (a.xyz[0] == b.xyz[0]) &&
		(a.xyz[1] == b.xyz[1]) &&
		(a.xyz[2] == b.xyz[2]);
}

int main() {
	string line;
	const string filename = "new_church_a";
	ifstream in(DATA_LOCATION + filename + ".zrs");//"./data/new_church_a.zrs");

	// Mesh Data
	// I have set Mesh to hold its vertices, normals, and texCoords as string
	// in order to avoid unncessary conversion from string to double (read in and store)
	// then double to string (write file).
	// However, I have made structs to be template just in case if you want to store it
	// as other data type than string.
	vector<Mesh<string>> meshes;
	
	// Material Data
	// Storing in a vector in case of using multiple materials
	vector<Material> materials;

	if (in.is_open())
	{
		while (getline(in, line)) {
			// search for material data
			if (line.find("material") != std::string::npos) {
				Material mt;
				while (getline(in, line)) {
					// search for material's name
					if (line.find("name") != std::string::npos) {
						vector<string> name;
						splitLine(line, "(\")\t", name);
						mt.name = name[1];
						
						// replace whitespace with underscore
						replace(mt.name.begin(), mt.name.end(), ' ', '_');
					}

					// Quick split and convert lambda function
					auto convert_colour_value = [&](string &line, string delim, Vector3<double> &col) {
						vector<int> colours;
						splitLine(line, delim, colours);
						if (colours.size() == 3) {
							for (size_t i = 0; i < colours.size(); i++) {
								col.xyz[i] = (double)colours[i] / 255.0;
							}
						}
					};

					// search for diffuse colour
					if (line.find("diffusecolour") != std::string::npos) {
						convert_colour_value(line, "diffusecolour(,)\t", mt.diffuse);
					}

					// search for ambient colour
					if (line.find("ambientcolour") != std::string::npos) {
						convert_colour_value(line, "ambientcolour(,)\t", mt.ambient);
					}

					// search for specular colour
					if (line.find("specularcolour") != std::string::npos) {
						convert_colour_value(line, "specularcolour(,)\t", mt.specular);

					}

					// search for opacity value
					if (line.find("opacity") != std::string::npos) {
						vector<double> opacity;
						splitLine(line, "opacity()\t", opacity);
						if (opacity.size() == 1) {
							mt.opacity = opacity[0];
						}
					}

					// End of material
					if (line == "\t\t)")
						break;
				}
				materials.push_back(mt);
			}

			// search for mesh data
			if (line.find("mesh") != std::string::npos) {
				Mesh<string> m;
			while (getline(in, line)) {
					// search for Mesh's name
					if (line.find("name") != std::string::npos) {
						vector<string> name;
						splitLine(line, "(\")\t", name);
						m.name = name[1];
					}

					// check if line contains vertex data
					if (line.find("vertex") != std::string::npos) {
						vector<string> data;
						splitLine(line, "vertex(,)\t", data);
						if (data.size() == 8) {
							m.vertices.push_back(Vector3<string>(data[0], data[1], data[2]));
							m.texCoords.push_back(Vector2<string>(data[3], data[4]));
							m.normals.push_back(Vector3<string>(data[5], data[6], data[7]));
						}
						else { // simple data structure check
							cout << "Error parsing vertex data! Please check ZRS format!" << endl;
							in.close();
							return -1;
						}
					}

					// check if line contains face (tri) data
					if (line.find("tri") != std::string::npos) {
						vector<int> data;
						splitLine(line, "tri(,)\t", data);
						if (data.size() == 3) {
							m.faces.push_back(Face(++data[0], ++data[1], ++data[2]));
						}
						else { // simple data structure check
							cout << "Error parsing face index data! Please check ZRS format!" << endl;
							in.close();
							return -1;
						}
					}

					// End of mesh data
					if (line == "\t\t)")
						break;
				}
				meshes.push_back(m);
			}

			// Find Object data (mesh -> material)
			if (line.find("object") != std::string::npos) {
				while (getline(in, line)) {
					// Find mesh under Part ()
					if (line.find("mesh") != std::string::npos) {
						vector<string> name;
						splitLine(line, "(\")\t", name);
						
						for (Mesh<string> &m : meshes) {
							if (m.name == name[1]) {
								// get one more line to extract material associated with the current mesh
								if (getline(in, line)) {
									// reuse vector since we don't need previous name vector anymore
									name.resize(0);
									splitLine(line, "(\")\t", name);

									// replace whitespace in material name
									replace(name[1].begin(), name[1].end(), ' ', '_');

									// find and assign material to mesh
									for (Material &mt : materials) {
										if (mt.name == name[1]) {
											m.mt = mt;
											m.hasMtl = true;
										}
									}
								}
								// exit once one mesh is found and material is set
								break;
							}
						}
						break;
					}
				}
			}
		}
		in.close();
	}

	// Remove duplicates in normal list
	for (Mesh<string> &m : meshes) {
		vector<Vector3<string>> newNormals;
		newNormals = m.normals;
		auto end = std::unique(newNormals.begin(), newNormals.end(), compare<string>);
		newNormals.resize(std::distance(newNormals.begin(), end));
		m.normals_ndp = newNormals;
	}

	// Wirte material to mtl format
	bool mtlLib = false;
	if (materials.size() > 0) {
		ofstream mtlFile(DATA_LOCATION + filename + ".mtl");
		if (mtlFile.is_open()) {
			for (Material mt : materials) {
				mtlFile << "newmtl " << mt.name << "\n\n";

				// set double precision to be 10 decimal places
				mtlFile << std::fixed << std::setprecision(10);

				// Ambient
				mtlFile << "Ka " << mt.ambient.xyz[0] << ' '
					<< mt.ambient.xyz[1] << ' '
					<< mt.ambient.xyz[2] << ' '
					<< '\n';

				// Diffuse
				mtlFile << "Kd " << mt.diffuse.xyz[0] << ' '
					<< mt.diffuse.xyz[1] << ' '
					<< mt.diffuse.xyz[2] << ' '
					<< '\n';

				// Specular
				mtlFile << "Ks " << mt.specular.xyz[0] << ' '
					<< mt.specular.xyz[1] << ' '
					<< mt.specular.xyz[2] << ' '
					<< '\n';

				// Reset fixed decimal place
				mtlFile << std::defaultfloat;

				// Opacity
				mtlFile << "d " << mt.opacity;
			}
			mtlFile.close();
			mtlLib = true;
		}
	}

	// Write mesh to obj format
	int lastIndex = 0;
	int lastNormIndex = 0;
	ofstream objFile(DATA_LOCATION + filename + ".obj");
	if (objFile.is_open())
	{
		if (mtlLib)
			objFile << "mtllib " << filename << ".mtl" << "\n\n";

		for (Mesh<string> m : meshes) {
			
			// Output name of mesh in a comment
			objFile << "#\n"
				<< "# Object " << m.name << '\n'
				<< "#\n";

			// Vertices
			for (Vector3<string> v : m.vertices) {
				objFile << "v " << v.xyz[0] << ' ' << v.xyz[1] << ' ' << v.xyz[2] << '\n';
			}
			objFile << "# " << m.vertices.size() << " vertices\n\n";

			// Normals
			for (Vector3<string> n : m.normals_ndp) {
				objFile << "vn " << n.xyz[0] << ' ' << n.xyz[1] << ' ' << n.xyz[2] << '\n';
			}
			objFile << "# " << m.normals_ndp.size() << " vertex normals\n\n";

			// TexCoords
			for (Vector2<string> t : m.texCoords) {
				objFile << "vt " << t.uv[0] << ' ' << t.uv[1] << '\n';
			}
			objFile << "# " << m.texCoords.size() << " texture coords\n\n";

			// Define mtlLib if any
			if (m.hasMtl) {
				objFile << "usemtl " << m.mt.name << '\n';
			}

			// Face indices
			for (Face f : m.faces) {
				int newNormIndices[3];

				// Find new index from non-duplicated normals list
				for (size_t i = 0; i < m.normals_ndp.size(); i++) {
					if (compare(m.normals[f.indices[0] - 1], m.normals_ndp[i])) {
						newNormIndices[0] = i + 1;
					}

					if (compare(m.normals[f.indices[1] - 1], m.normals_ndp[i])) {
						newNormIndices[1] = i + 1;
					}

					if (compare(m.normals[f.indices[2] - 1], m.normals_ndp[i])) {
						newNormIndices[2] = i + 1;
					}
				}

				objFile << "f " << f.indices[0] + lastIndex << '/' << f.indices[0] + lastIndex << '/' << newNormIndices[0] + lastNormIndex
					<< ' ' << f.indices[1] + lastIndex << '/' << f.indices[1] + lastIndex << '/' << newNormIndices[1] + lastNormIndex
					<< ' ' << f.indices[2] + lastIndex << '/' << f.indices[2] + lastIndex << '/' << newNormIndices[2] + lastNormIndex
					<< '\n';
			}
			objFile << "# " << m.faces.size() << " faces\n\n";

			lastIndex += m.vertices.size();
			lastNormIndex += m.normals_ndp.size();
		}
		objFile.close();
	}

	return 0;
}