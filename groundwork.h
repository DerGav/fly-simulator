#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <io.h>
#include <algorithm>    // std::min_element, std::max_element
#include "resource.h"
using namespace std;

#define SPHERE	0
#define BOX		1
#define COORD   2
typedef int BOUNDARY_TYPE;



struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
	XMFLOAT3 Norm;
};


struct ObjectBox
{
	float max_x;
	float min_x;
	float max_y;
	float min_y;
	float max_z;
	float min_z;
};

class Boundary
{
public:
	virtual ~Boundary() {};
	//virtual Boundary& operator=(const Boundary& other) = 0;
	virtual float calculate_distance(XMFLOAT3 pos) = 0;
	virtual bool check_collision(XMFLOAT3 pos) = 0;
	virtual void transform_boundary(XMMATRIX& M, float scale_factor) = 0;

private:

};

class Sphere_Boundary : public Boundary
{
public:
	Sphere_Boundary(XMFLOAT3 center, float radius);
	//Sphere_Boundary(const Sphere_Boundary& other);
	//Boundary& operator=(const Boundary& other);
	//Sphere_Boundary& operator=(const Sphere_Boundary& other);
	float calculate_distance(XMFLOAT3 pos);
	virtual bool check_collision(XMFLOAT3 pos);
	virtual void transform_boundary(XMMATRIX& M, float scale_factor);

	XMFLOAT3 center;
	float radius;

};

class Box_Boundary : public Boundary
{
public:
	Box_Boundary(XMFLOAT3 max_x, XMFLOAT3 max_y, XMFLOAT3 max_z, XMFLOAT3 min_x, XMFLOAT3 min_y, XMFLOAT3 min_z);
	/*Box_Boundary(const Box_Boundary& other);
	Boundary& operator=(const Boundary& other);
	Box_Boundary& operator=(const Box_Boundary& other);*/
	float calculate_distance(XMFLOAT3 pos);
	virtual bool check_collision(XMFLOAT3 pos);
	virtual void transform_boundary(XMMATRIX& M, float scale_factor);

	XMFLOAT3 max_x;
	XMFLOAT3 min_x;
	XMFLOAT3 max_y;
	XMFLOAT3 min_y;
	XMFLOAT3 max_z;
	XMFLOAT3 min_z;
};

class Coord_Boundary : public Boundary
{
public:
	float calculate_distance(XMFLOAT3 pos);
	virtual bool check_collision(XMFLOAT3 pos);
	virtual void transform_boundary(XMMATRIX& M, float scale_factor);

};


// struct for object bounding sphere
struct ObjectSphere
{
	XMFLOAT3 center;
	float radius;
};

// struct for object bounding sphere using XMVECTOR -> easier to transform with XMMATRIX
struct ObjectSphereVec
{
	XMVECTOR center;
	XMVECTOR radius;
};

//forward declarations
class Model;
class camera;

// collision detection class
class Collision_Detection
{
public:
	// store the object bounding spheres here and leave them unchanged
	// similar to the way that model in vertex buffer remains unchanged 
	vector<ObjectSphereVec> objectSphereVecs;

	// 'push' the transformed bounding spheres to this vector which is a member of camera
	vector<ObjectSphere>*   p_objectSpheres;

	//maybe move collision check here in future...
	//bool check_collision(XMFLOAT3 pos);

	// push model to the vector in camera
	int push_model(Model* model);

	// store the bounding sphere in the array with the unchanged bounding spheres
	int store_model(ObjectSphereVec& objSphV);
};

// forward declaration
//new parameters:
// - collision_detection: pass pointer to object so that bounding sphere can be stored there
// - index: pass pointer to int, so that index in the bounding sphere vector can be stored there
//bool Load3DS(char *filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count, Collision_Detection* collision_detection, int* index, BOUNDARY_TYPE b);

bool Load3DS(char *filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count, BOUNDARY_TYPE b, Model* model, bool g);
bool LoadCatmullClark(LPCTSTR filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count);

// encapsulates vertexbuffer and some other information needed for collision detection in one class
class Model
{
public:
	Model();
	Model(const Model& other);
	Model& operator=(const Model& other);
	~Model();
	//members
	ID3D11Buffer* vertex_buffer;
	int vertex_count;
	ID3D11ShaderResourceView* g_pTexture;

	//int index; 		//index in both the collision detection vectors

	Boundary* boundary;

	//loads a model from 3ds file into vertexbuffer and calculates and stores bounding sphere
	void load_Model(char* filename, ID3D11Device* device, BOUNDARY_TYPE b, bool g);

	//update the models bounding sphere, with the worldmatrix M
	//scaling matrix must be passed seperately at this point to correctly scale the radius of the sphere..
	//call this before every draw call
	//void update_objectSphere(Collision_Detection* c_d, XMMATRIX& M, XMMATRIX& S);

};

class ConstantBuffer
{
public:
	ConstantBuffer()
	{
		info = XMFLOAT4(1, 1, 1, 1);
	}
	XMMATRIX World;
	XMMATRIX View;
	XMMATRIX Projection;
	XMMATRIX LightView;
	XMFLOAT4 info;
};


//********************************************
//********************************************
class StopWatchMicro_
{
private:
	LARGE_INTEGER last, frequency;
public:
	StopWatchMicro_()
	{
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&last);

	}
	long double elapse_micro()
	{
		LARGE_INTEGER now, dif;
		QueryPerformanceCounter(&now);
		dif.QuadPart = now.QuadPart - last.QuadPart;
		long double fdiff = (long double)dif.QuadPart;
		fdiff /= (long double)frequency.QuadPart;
		return fdiff*1000000.;
	}
	long double elapse_milli()
	{
		elapse_micro() / 1000.;
	}
	void start()
	{
		QueryPerformanceCounter(&last);
	}
};
//**********************************
class billboard
{
public:
	billboard()
	{
		position = XMFLOAT3(0, 0, 0);
		scale = 1;
		transparency = 1;
	}
	XMFLOAT3 position; //obvious
	float scale;		//in case it can grow
	float transparency; //for later use
	XMMATRIX get_matrix(XMMATRIX &ViewMatrix)
	{

		XMMATRIX view, R, T, S;
		view = ViewMatrix;
		//eliminate camera translation:
		view._41 = view._42 = view._43 = 0.0;
		XMVECTOR det;
		R = XMMatrixInverse(&det, view);//inverse rotation
		T = XMMatrixTranslation(position.x, position.y, position.z);
		S = XMMatrixScaling(scale, scale, scale);
		return S*R*T;
	}

	XMMATRIX get_matrix_y(XMMATRIX &ViewMatrix) //enemy-type
	{

	}
};

//*****************************************
class bitmap
{

public:
	BYTE *image;
	int array_size;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	bitmap()
	{
		image = NULL;
	}
	~bitmap()
	{
		if (image)
			delete[] image;
		array_size = 0;
	}
	bool read_image(char *filename)
	{
		ifstream bmpfile(filename, ios::in | ios::binary);
		if (!bmpfile.is_open()) return FALSE;	// Error opening file
		bmpfile.read((char*)&bmfh, sizeof(BITMAPFILEHEADER));
		bmpfile.read((char*)&bmih, sizeof(BITMAPINFOHEADER));
		bmpfile.seekg(bmfh.bfOffBits, ios::beg);
		//make the array
		if (image)delete[] image;
		int size = bmih.biWidth*bmih.biHeight * 3;
		image = new BYTE[size];//3 because red, green and blue, each one byte
		bmpfile.read((char*)image, size);
		array_size = size;
		bmpfile.close();
		check_save();
		return TRUE;
	}
	BYTE get_pixel(int x, int y, int color_offset) //color_offset = 0,1 or 2 for red, green and blue
	{
		int array_position = x * 3 + y* bmih.biWidth * 3 + color_offset;
		if (array_position >= array_size) return 0;
		if (array_position < 0) return 0;
		return image[array_position];
	}
	void check_save()
	{
		ofstream nbmpfile("newpic.bmp", ios::out | ios::binary);
		if (!nbmpfile.is_open()) return;
		nbmpfile.write((char*)&bmfh, sizeof(BITMAPFILEHEADER));
		nbmpfile.write((char*)&bmih, sizeof(BITMAPINFOHEADER));
		//offset:
		int rest = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER);
		if (rest > 0)
		{
			BYTE *r = new BYTE[rest];
			memset(r, 0, rest);
			nbmpfile.write((char*)&r, rest);
		}
		nbmpfile.write((char*)image, array_size);
		nbmpfile.close();

	}
};
////////////////////////////////////////////////////////////////////////////////
//lets assume a wall is 10/10 big!
#define FULLWALL 2
#define HALFWALL 1
class wall
{
public:
	XMFLOAT3 position;
	int texture_no;
	int rotation; //0,1,2,3,4,5 ... facing to z, x, -z, -x, y, -y
	wall()
	{
		texture_no = 0;
		rotation = 0;
		position = XMFLOAT3(0, 0, 0);
	}
	XMMATRIX get_matrix()
	{
		XMMATRIX R, T, T_offset;
		R = XMMatrixIdentity();
		T_offset = XMMatrixTranslation(0, 0, -HALFWALL);
		T = XMMatrixTranslation(position.x, position.y, position.z);
		switch (rotation)//0,1,2,3,4,5 ... facing to z, x, -z, -x, y, -y
		{
		default:
		case 0:	R = XMMatrixRotationY(XM_PI);		T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
		case 1: R = XMMatrixRotationY(XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
		case 2:										T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
		case 3: R = XMMatrixRotationY(-XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
		case 4: R = XMMatrixRotationX(XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, -HALFWALL); break;
		case 5: R = XMMatrixRotationX(-XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, -HALFWALL); break;
		}
		return T_offset * R * T;
	}
};
//********************************************************************************************
class level
{
private:
	bitmap leveldata;
	vector<wall*> walls;						//all wall positions
	vector<ID3D11ShaderResourceView*> textures;	//all wall textures
	void process_level()
	{
		//we have to get the level to the middle:
		int x_offset = (leveldata.bmih.biWidth / 2)*FULLWALL;

		//lets go over each pixel without the borders!, only the inner ones
		for (int yy = 1; yy < (leveldata.bmih.biHeight - 1); yy++)
			for (int xx = 1; xx < (leveldata.bmih.biWidth - 1); xx++)
			{
				//wall information is the interface between pixels:
				//blue to something not blue: wall. texture number = 255 - blue
				//green only: floor. texture number = 255 - green
				//red only: ceiling. texture number = 255 - red
				//green and red: floor and ceiling ............
				BYTE red, green, blue;

				blue = leveldata.get_pixel(xx, yy, 0);
				green = leveldata.get_pixel(xx, yy, 1);
				red = leveldata.get_pixel(xx, yy, 2);

				if (blue > 0)//wall possible
				{
					int texno = 255 - blue;
					BYTE left_red = leveldata.get_pixel(xx - 1, yy, 2);
					BYTE left_green = leveldata.get_pixel(xx - 1, yy, 1);
					BYTE right_red = leveldata.get_pixel(xx + 1, yy, 2);
					BYTE right_green = leveldata.get_pixel(xx + 1, yy, 1);
					BYTE top_red = leveldata.get_pixel(xx, yy + 1, 2);
					BYTE top_green = leveldata.get_pixel(xx, yy + 1, 1);
					BYTE bottom_red = leveldata.get_pixel(xx, yy - 1, 2);
					BYTE bottom_green = leveldata.get_pixel(xx, yy - 1, 1);

					if (left_red>0 || left_green > 0)//to the left
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 3, texno);
					if (right_red>0 || right_green > 0)//to the right
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 1, texno);
					if (top_red>0 || top_green > 0)//to the top
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 2, texno);
					if (bottom_red>0 || bottom_green > 0)//to the bottom
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 0, texno);
				}
				if (red > 0)//ceiling
				{
					int texno = 255 - red;
					init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 5, texno);
				}
				if (green > 0)//floor
				{
					int texno = 255 - green;
					init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 4, texno);
				}
			}
	}
	void init_wall(XMFLOAT3 pos, int rotation, int texture_no)
	{
		wall *w = new wall;
		walls.push_back(w);
		w->position = pos;
		w->rotation = rotation;
		w->texture_no = texture_no;
	}
public:
	level()
	{
	}
	void init(char *level_bitmap)
	{
		if (!leveldata.read_image(level_bitmap))return;
		process_level();
	}
	bool init_texture(ID3D11Device* pd3dDevice, LPCWSTR filename)
	{
		// Load the Texture
		ID3D11ShaderResourceView *texture;
		HRESULT hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, filename, NULL, NULL, &texture, NULL);
		if (FAILED(hr))
			return FALSE;
		textures.push_back(texture);
		return TRUE;
	}
	ID3D11ShaderResourceView *get_texture(int no)
	{
		if (no < 0 || no >= textures.size()) return NULL;
		return textures[no];
	}
	XMMATRIX get_wall_matrix(int no)
	{
		if (no < 0 || no >= walls.size()) return XMMatrixIdentity();
		return walls[no]->get_matrix();
	}
	int get_wall_count()
	{
		return walls.size();
	}
	void render_level(ID3D11DeviceContext* ImmediateContext, ID3D11Buffer *vertexbuffer_wall, XMMATRIX *view, XMMATRIX *projection, ID3D11Buffer* dx_cbuffer)
	{
		//set up everything for the waqlls/floors/ceilings:
		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		ImmediateContext->IASetVertexBuffers(0, 1, &vertexbuffer_wall, &stride, &offset);
		ConstantBuffer constantbuffer;
		constantbuffer.View = XMMatrixTranspose(*view);
		constantbuffer.Projection = XMMatrixTranspose(*projection);
		XMMATRIX wall_matrix, S;
		ID3D11ShaderResourceView* tex;
		//S = XMMatrixScaling(FULLWALL, FULLWALL, FULLWALL);
		S = XMMatrixScaling(1, 1, 1);
		for (int ii = 0; ii < walls.size(); ii++)
		{
			wall_matrix = walls[ii]->get_matrix();
			int texno = walls[ii]->texture_no;
			if (texno >= textures.size())
				texno = 0;
			tex = textures[texno];
			wall_matrix = wall_matrix;// *S;

			constantbuffer.World = XMMatrixTranspose(wall_matrix);

			ImmediateContext->UpdateSubresource(dx_cbuffer, 0, NULL, &constantbuffer, 0, 0);
			ImmediateContext->VSSetConstantBuffers(0, 1, &dx_cbuffer);
			ImmediateContext->PSSetConstantBuffers(0, 1, &dx_cbuffer);
			ImmediateContext->PSSetShaderResources(0, 1, &tex);
			ImmediateContext->Draw(6, 0);
		}
	}
};



class camera
{
private:

public:
	int w, s, a, d, q, e;
	float controlledspeed;
	XMFLOAT3 position;
	XMFLOAT3 rotation;

	//vector with all the spheres to which distance has to be calculated
	vector<Model>* models;

	camera()
	{
		w = s = a = d = 0;
		position = XMFLOAT3(0, 0, 0);
		rotation = XMFLOAT3(0, 0, 0);
		controlledspeed = 0.3;

	}
	//return pointer to vector
	//vector<ObjectSphere>* getObjectSpheres()
	//{
	//	return &(this->objSphs);
	//}
	void animation(float elapsed_microseconds)
	{
		XMMATRIX Ry, Rx, T;
		Ry = XMMatrixRotationY(-rotation.y);
		Rx = XMMatrixRotationX(-rotation.x);

		XMFLOAT3 forward = XMFLOAT3(0, 0, 1);
		XMVECTOR f = XMLoadFloat3(&forward);
		f = XMVector3TransformCoord(f, Rx*Ry);
		XMStoreFloat3(&forward, f);
		XMFLOAT3 side = XMFLOAT3(1, 0, 0);
		XMVECTOR si = XMLoadFloat3(&side);
		si = XMVector3TransformCoord(si, Rx*Ry);
		XMStoreFloat3(&side, si);

		float speed = elapsed_microseconds / 100000.0;


		//if (!check_collision())
		//{
		//	position.x -= forward.x * speed / 10;
		//	position.y -= forward.y * speed / 10;
		//	position.z -= forward.z * speed / 10;
		//}



		/*if (w)
		{
		position.x -= forward.x * speed;
		position.y -= forward.y * speed;
		position.z -= forward.z * speed;
		}
		if (s)
		{
		position.x += forward.x * speed;
		position.y += forward.y * speed;
		position.z += forward.z * speed;
		}
		if (d)
		{
		position.x -= side.x * 0.01;
		position.y -= side.y * 0.01;
		position.z -= side.z * 0.01;
		}
		if (a)
		{
		position.x += side.x * 0.01;
		position.y += side.y * 0.01;
		position.z += side.z * 0.01;
		}*/

		XMFLOAT3 possible_position = position;
		//if (w)
		{
			possible_position.x -= forward.x * speed * controlledspeed;
			possible_position.y -= forward.y * speed * controlledspeed;
			possible_position.z -= forward.z * speed * controlledspeed;
		}
		/*		if (s)
		{
		possible_position.x += forward.x * speed;
		possible_position.y += forward.y * speed;
		possible_position.z += forward.z * speed;
		}

		if (d)
		{
		possible_position.x -= side.x * 0.01;
		possible_position.y -= side.y * 0.01;
		possible_position.z -= side.z * 0.01;
		}
		if (a)
		{
		possible_position.x += side.x * 0.01;
		possible_position.y += side.y * 0.01;
		possible_position.z += side.z * 0.01;
		}*/
		//check for collision at new position before setting
		if (!check_collision(possible_position))
		{
			position = possible_position;
		}
		//for now move player back a bit in case of collision
		else
		{
			position.x += forward.x;
			position.y += forward.y;
			position.z += forward.z;
		}

	}
	XMMATRIX get_matrix(XMMATRIX *view)
	{
		XMMATRIX Rx, Ry, T;
		Rx = XMMatrixRotationX(rotation.x);
		Ry = XMMatrixRotationY(rotation.y);
		T = XMMatrixTranslation(position.x, position.y, position.z);
		return T*(*view)*Ry*Rx;
	}
	bool check_collision(XMFLOAT3 pos)
	{
		//calculate distance to the center points of all the bounding spheres
		for (std::vector<Model>::iterator it = models->begin(); it != models->end(); ++it)
		{
			if (it->boundary->check_collision(pos))
			{
				return true;
			}
			//const XMFLOAT3 c = XMFLOAT3(it->center.x, it->center.y, it->center.z);
			//const XMFLOAT3 pos = XMFLOAT3(-position.x, -position.y, -position.z);
			//XMFLOAT3 d_vec = XMFLOAT3(
			//	pos.x - it->center.x,
			//	pos.y - it->center.y,
			//	pos.z - it->center.z
			//);

			//float d = sqrtf(d_vec.x*d_vec.x + d_vec.y*d_vec.y + d_vec.z*d_vec.z);

			////if the distance to a sphere's center is smaller than the sphere's radius, report collision
			//if (d < it->radius)
			//	return true;
		}

		return false;
	}
};




class bullet
{
public:
	XMFLOAT3 pos, imp;
	bullet()
	{
		pos = imp = XMFLOAT3(0, 0, 0);
	}
	XMMATRIX getmatrix(float elapsed, XMMATRIX &view)
	{

		pos.x = pos.x + imp.x *(elapsed / 100000.0);
		pos.y = pos.y + imp.y *(elapsed / 100000.0);
		pos.z = pos.z + imp.z *(elapsed / 100000.0);

		XMMATRIX R, T;
		R = view;
		R._41 = R._42 = R._43 = 0.0;
		XMVECTOR det;
		R = XMMatrixInverse(&det, R);
		T = XMMatrixTranslation(pos.x, pos.y, pos.z);

		return R * T;
	}
};




float Vec3Length(const XMFLOAT3 &v);
float Vec3Dot(XMFLOAT3 a, XMFLOAT3 b);
XMFLOAT3 Vec3Cross(XMFLOAT3 a, XMFLOAT3 b);
XMFLOAT3 Vec3Normalize(const  XMFLOAT3 &a);
XMFLOAT3 operator+(const XMFLOAT3 lhs, const XMFLOAT3 rhs);
XMFLOAT3 operator-(const XMFLOAT3 lhs, const XMFLOAT3 rhs);
XMFLOAT3 mul(XMFLOAT3 vec, XMMATRIX &M);

bool Load3DS(char *filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count);
//bool Load3DS(char *filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count, vector<ObjectSphereVec>* objectSpheres);