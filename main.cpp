#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

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

template <typename T>
struct Mesh {
	string name;
	vector<Vector3<T>> vertices;
	vector<Vector2<T>> texCoords;
	vector<Vector3<T>> normals; // with duplicates
	vector<Vector3<T>> normals_ndp; // without duplciates
	vector<Face> faces;
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

// For Face indices (always going to be int)
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
			out.push_back(++value);
		}
		prev_pos = pos + 1;
	}
	// Last number
	if (prev_pos < str.length()) {
		f = str.substr(prev_pos, std::string::npos);
		value = stoi(f.c_str());
		out.push_back(++value);
	}
}

template <typename T>
bool compare(Vector3<T> a, Vector3<T> b) {
	return (a.xyz[0] == b.xyz[0]) &&
		(a.xyz[1] == b.xyz[1]) &&
		(a.xyz[2] == b.xyz[2]);
}

int main() {
	string line;
	ifstream in("./data/new_church_a.zrs");

	// Mesh Data
	// I have set Mesh to hold its vertices, normals, and texCoords as string
	// in order to avoid unncessary conversion from string to double (read in and store)
	// then double to string (write file).
	// However, I have made structs to be template just in case if you want to store it
	// as other data type than string.
	vector<Mesh<string>> meshes;

	if (in.is_open())
	{
		while (getline(in, line)) {
			if (line.find("mesh") != std::string::npos) {
				Mesh<string> m;
				while (getline(in, line)) {
					// set loop break point to be on 'kflcount' line
					if (line.find("kflcount") == std::string::npos) {	

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

						// 168.38256
						// check if line contains face (tri) data
						if (line.find("tri") != std::string::npos) {
							vector<int> data;
							splitLine(line, "tri(,)\t", data);
							if (data.size() == 3) {
								m.faces.push_back(Face(data[0], data[1], data[2]));
							}
						}
					}
					else
						break;

				}
				meshes.push_back(m);
			}

			if (line.find("object") != std::string::npos) break;
		}
		in.close();
	}

	// Remove duplicates in normal list
	for (Mesh<string> m : meshes) {
		vector<Vector3<string>> newNormals;
		newNormals = m.normals;
		auto end = std::unique(newNormals.begin(), newNormals.end(), compare<string>);
		newNormals.resize(std::distance(newNormals.begin(), end));
		m.normals_ndp = newNormals;
	}


	// Write to obj format
	int lastIndex = 0;
	ofstream objFile("./data/new_church_a.obj");
	if (objFile.is_open())
	{
		for (Mesh<string> m : meshes) {
			
			objFile << "#\n"
				<< "# Object " << m.name << '\n'
				<< "#\n";

			// Vertices
			for (Vector3<string> v : m.vertices) {
				objFile << "v " << v.xyz[0] << ' ' << v.xyz[1] << ' ' << v.xyz[2] << '\n';
			}

			objFile << "# " << m.vertices.size() << " vertices\n\n";

			// Normals
			for (Vector3<string> n : m.normals) {
				objFile << "vn " << n.xyz[0] << ' ' << n.xyz[1] << ' ' << n.xyz[2] << '\n';
			}

			objFile << "# " << m.normals.size() << " vertex normals\n\n";

			// TexCoords
			for (Vector2<string> t : m.texCoords) {
				objFile << "vt " << t.uv[0] << ' ' << t.uv[1] << '\n';
			}

			objFile << "# " << m.texCoords.size() << " texture coords\n\n";

			for (Face f : m.faces) {
				objFile << "f " << f.indices[0] + lastIndex << '/' << f.indices[0] + lastIndex << '/' << f.indices[0] + lastIndex
					<< ' ' << f.indices[1] + lastIndex << '/' << f.indices[1] + lastIndex << '/' << f.indices[1] + lastIndex
					<< ' ' << f.indices[2] + lastIndex << '/' << f.indices[2] + lastIndex << '/' << f.indices[2] + lastIndex
					<< '\n';
			}
			objFile << "# " << m.faces.size() << " faces\n\n";
			lastIndex = m.vertices.size();
		}
		objFile.close();
	}

	return 0;
}