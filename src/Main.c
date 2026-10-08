#include "/home/codeleaded/System/Static/Library/WindowEngine.h"
#include "/home/codeleaded/System/Static/Library/Geometry3D.h"
#include "/home/codeleaded/System/Static/Library/Lib3D_Cube.h"
#include "/home/codeleaded/System/Static/Library/Lib3D_Mathlib.h"
#include "/home/codeleaded/System/Static/Library/Lib3D_Mesh.h"
#include "/home/codeleaded/System/Static/Library/Lib3D_World3D.h"

#define G				1.0f
#define F_MAX			10000.0f
#define FAKTOR			1.0f

#define FIELDX			40
#define FIELDZ			40

#define GRID_SCALE_X	3.0f
#define GRID_SCALE_Z	3.0f


typedef struct Sphere{
	Vec3D p;
	Vec3D v;
	Vec3D a;
	float r;
	float m;
	Pixel c;
} Sphere;

Sphere Sphere_New(Vec3D p,Vec3D v,Vec3D a,float r,float m,Pixel c){
	Sphere s;
	s.p = p;
	s.v = v;
	s.a = a;
	s.r = r;
	s.m = m;
	s.c = c;
	return s;
}
float Sphere_Gravity(const Sphere* s,Vec3D p){
	const Vec3D dir = Vec3D_Sub(p,s->p);
	const float r = Vec3D_Length(dir);
	return G * s->m / (r * r);
	//return F32_Clamp(G * s->m / (r * r),0.0f,F_MAX);
}
void Sphere_AddGravity(Sphere* s,Sphere* other){
	const float f = Sphere_Gravity(other,s->p);
	const Vec3D a = Vec3D_Mul(Vec3D_Normalise(Vec3D_Sub(other->p,s->p)),f);
	s->a = Vec3D_Add(s->a,a);
}
char Sphere_isCollision(const Sphere* s,Sphere* other){
	const float d = Vec3D_Length(Vec3D_Sub(other->p,s->p));
    return d < (s->r + other->r);
}
void Sphere_Collision(Sphere* s,Sphere* other){
	if(Sphere_isCollision(s,other)){
		const Vec3D d = Vec3D_Sub(s->p,other->p);
		const float h = Vec3D_Length(d);
		const float Overlap = 0.5f * (h - s->r - other->r);
		s->p = Vec3D_Sub(s->p,Vec3D_Div(Vec3D_Mul(d,Overlap),h));
		other->p = Vec3D_Add(other->p,Vec3D_Div(Vec3D_Mul(d,Overlap),h));
	}
}
void Sphere_Update(Sphere* s,float ElapsedTime){
	s->v = Vec3D_Add(s->v,Vec3D_Mul(s->a,ElapsedTime));
	s->p = Vec3D_Add(s->p,Vec3D_Mul(s->v,ElapsedTime));
	
	//s->p.y = 0.5f - FAKTOR * Vec3D_Length(s->a);
	s->a = Vec3D_Null();
}
void Sphere_Render(const Sphere* s,Vector* tris){
	for(float i = 0.0f;i<2 * F32_PI;i+=0.2f){
		M4x4D matX = Matrix_MakeRotationX(i);
		M4x4D matXt = Matrix_MakeRotationX(i+0.2f);

		for(float j = 0.0f;j<2 * F32_PI;j+=0.2f){
			M4x4D matY = Matrix_MakeRotationY(j);
			M4x4D matYt = Matrix_MakeRotationY(j+0.2f);

			M4x4D mat00 = Matrix_MultiplyMatrix(matX,	matY);
			M4x4D mat10 = Matrix_MultiplyMatrix(matXt,	matY);
			M4x4D mat01 = Matrix_MultiplyMatrix(matX,	matYt);
			M4x4D mat11 = Matrix_MultiplyMatrix(matXt,	matYt);
			
			Vec3D v00 = Matrix_MultiplyVector(mat00,Vec3D_New(0.0f,0.0f,1.0f));
			Vec3D v10 = Matrix_MultiplyVector(mat10,Vec3D_New(0.0f,0.0f,1.0f));
			Vec3D v01 = Matrix_MultiplyVector(mat01,Vec3D_New(0.0f,0.0f,1.0f));
			Vec3D v11 = Matrix_MultiplyVector(mat11,Vec3D_New(0.0f,0.0f,1.0f));

			Tri3D t1 = { .p = { v00,v10,v11 }, .c = s->c };
			Tri3D_CalcNorm(&t1);
			Tri3D_ShadeNorm(&t1,Vec3D_New(0.5f,0.6f,0.7f));
			Tri3D_Scale(&t1,s->r);
			Tri3D_Offset(&t1,s->p);

			Tri3D t2 = { .p = { v00,v11,v01 }, .c = s->c };
			Tri3D_CalcNorm(&t2);
			Tri3D_ShadeNorm(&t2,Vec3D_New(0.5f,0.6f,0.7f));
			Tri3D_Scale(&t2,s->r);
			Tri3D_Offset(&t2,s->p);

			Vector_Push(tris,&t1);
			Vector_Push(tris,&t2);
		}
	}
}


Camera cam;
World3D world;
int Mode = 0;
int Menu = 0;
float Speed = 4.0f;
Vector spheres;

void Menu_Set(int m){
	if(Menu==0 && m==1){
		AlxWindow_Mouse_SetInvisible(&window);
		SetMouse((Vec2){ GetWidth() / 2,GetHeight() / 2 });
	}
	if(Menu==1 && m==0){
		AlxWindow_Mouse_SetVisible(&window);
	}
	
	Menu = m;
}

void Setup(AlxWindow* w){
	Menu_Set(1);

	cam = Camera_Make(
		(Vec3D){ 0.0f,10.0f,-25.0f,1.0f },
		(Vec3D){ F32_PI025,0.0f,0.0f,1.0f },
		90.0f
	);

	world = World3D_Make(
		Matrix_MakeWorld((Vec3D){ 0.0f,0.0f,0.0f,1.0f },(Vec3D){ 0.0f,0.0f,0.0f,1.0f }),
		Matrix_MakePerspektive(cam.p,cam.up,cam.a),
		Matrix_MakeProjection(cam.fov,(float)GetHeight() / (float)GetWidth(),0.1f,1000.0f)
	);
	world.normal = WORLD3D_NORMAL_CAP;

	spheres = Vector_New(sizeof(Sphere));

	Vector_Push(&spheres,(Sphere[]){ Sphere_New(
		Vec3D_New(0.0f,0.0f,0.0f),
		Vec3D_New(0.0f,0.0f,0.0f),
		Vec3D_New(0.0f,0.0f,0.0f),
		1.0f,
		10000.0f,
		RED
	)});
}
void Update(AlxWindow* w){
	if(Menu==1){
		Camera_Focus(&cam,GetMouseBefore(),GetMouse(),GetScreenRect().d);
		Camera_Update(&cam);
		SetMouse((Vec2){ GetWidth() / 2,GetHeight() / 2 });
	}
	
	if(Stroke(ALX_MOUSE_L).PRESSED){
		const Sphere* const sun = (Sphere*)Vector_Get(&spheres,0);
		
		const Vec3 cam_pos = Vec3_New(cam.p.x,cam.p.y,cam.p.z);
		const Vec3 cam_dir = Vec3_New(cam.ld.x,cam.ld.y,cam.ld.z);
		Vec3 pos = Vec3_New(0.0f,0.0f,0.0f);

		if(Intersection_Ray3_Plane3(cam_pos,cam_dir,Vec3_New(0.0f,0.0f,0.0f),Vec3_New(0.0f,1.0f,0.0f),&pos)){
			const Vec3D posi = { pos.x,pos.y,pos.z };
			const Vec3D delta = Vec3D_Sub(sun->p,posi);
			const Vec3D dir = Vec3D_Normalise(delta);
			const Vec3D ndir = Matrix_MultiplyVector(Matrix_MakeRotationY(F32_PI05),dir);

			const float mass = 10.0f;
			const float r = Vec3D_Length(delta);
			const float a = Sphere_Gravity(sun,posi);
			const float orbit_v = sqrtf(a * r);
			
			Vector_Push(&spheres,(Sphere[]){ Sphere_New(
				posi,
				Vec3D_Mul(ndir,orbit_v),
				Vec3D_New(0.0f,0.0f,0.0f),
				1.0f,
				mass,
				GREEN
			)});
		}
	}

	if(Stroke(ALX_KEY_ESC).PRESSED)
		Menu_Set(!Menu);

	if(Stroke(ALX_KEY_Z).PRESSED)
		Mode = Mode < 2 ? Mode + 1 : 0;

	if(Stroke(ALX_KEY_W).DOWN)
		cam.p = Vec3D_Add(cam.p,Vec3D_Mul(cam.ld,Speed * w->ElapsedTime));
	if(Stroke(ALX_KEY_S).DOWN)
		cam.p = Vec3D_Sub(cam.p,Vec3D_Mul(cam.ld,Speed * w->ElapsedTime));
	if(Stroke(ALX_KEY_A).DOWN)
		cam.p = Vec3D_Add(cam.p,Vec3D_Mul(cam.sd,Speed * w->ElapsedTime));
	if(Stroke(ALX_KEY_D).DOWN)
		cam.p = Vec3D_Sub(cam.p,Vec3D_Mul(cam.sd,Speed * w->ElapsedTime));
	if(Stroke(ALX_KEY_R).DOWN)
		cam.p.y += Speed * w->ElapsedTime;
	if(Stroke(ALX_KEY_F).DOWN)
		cam.p.y -= Speed * w->ElapsedTime;


	for(int i = 0;i<spheres.size;i++){
		Sphere* s = (Sphere*)Vector_Get(&spheres,i);
		
		for(int j = 0;j<spheres.size;j++){
			if(i==j) continue;

			Sphere* other = (Sphere*)Vector_Get(&spheres,j);
			Sphere_Collision(s,other);
			Sphere_AddGravity(s,other);
		}
	}
	for(int i = 0;i<spheres.size;i++){
		Sphere* s = (Sphere*)Vector_Get(&spheres,i);
		Sphere_Update(s,w->ElapsedTime);
	}

	World3D_Set_Model(&world,Matrix_MakeWorld((Vec3D){ 0.0f,0.0f,0.0f,1.0f },(Vec3D){ 0.0f,0.0f,0.0f,1.0f }));
	World3D_Set_View(&world,Matrix_MakePerspektive(cam.p,cam.up,cam.a));
	World3D_Set_Proj(&world,Matrix_MakeProjection(cam.fov,(float)GetHeight() / (float)GetWidth(),0.1f,1000.0f));
	
	Vector_Clear(&world.trisIn);
	for(int i = 0;i<spheres.size;i++){
		Sphere* s = (Sphere*)Vector_Get(&spheres,i);
		Sphere_Render(s,&world.trisIn);
	}

	for(int i = -FIELDX;i<FIELDX;i++){
		for(int j = -FIELDZ;j<FIELDZ;j++){
			Vec3D v00 = Vec3D_New(GRID_SCALE_X * (i + 0),	0.0f,GRID_SCALE_Z * (j + 0));
			Vec3D v10 = Vec3D_New(GRID_SCALE_X * (i + 1),	0.0f,GRID_SCALE_Z * (j + 0));
			Vec3D v01 = Vec3D_New(GRID_SCALE_X * (i + 0),	0.0f,GRID_SCALE_Z * (j + 1));
			Vec3D v11 = Vec3D_New(GRID_SCALE_X * (i + 1),	0.0f,GRID_SCALE_Z * (j + 1));

			for(int k = 0;k<spheres.size;k++){
				Sphere* s = (Sphere*)Vector_Get(&spheres,k);
				v00.y -= FAKTOR * Sphere_Gravity(s,Vec3D_New(v00.x,-1.0f,v00.z));
				v10.y -= FAKTOR * Sphere_Gravity(s,Vec3D_New(v10.x,-1.0f,v10.z));
				v01.y -= FAKTOR * Sphere_Gravity(s,Vec3D_New(v01.x,-1.0f,v01.z));
				v11.y -= FAKTOR * Sphere_Gravity(s,Vec3D_New(v11.x,-1.0f,v11.z));
			}

			Tri3D t1 = { .p = { v00,v11,v10 }, .c = WHITE };
			Tri3D_CalcNorm(&t1);
			Tri3D_ShadeNorm(&t1,Vec3D_New(0.5f,0.6f,0.7f));

			Tri3D t2 = { .p = { v00,v01,v11 }, .c = WHITE };
			Tri3D_CalcNorm(&t2);
			Tri3D_ShadeNorm(&t2,Vec3D_New(0.5f,0.6f,0.7f));

			Vector_Push(&world.trisIn,&t1);
			Vector_Push(&world.trisIn,&t2);
		}
	}

	Clear(LIGHT_BLUE);

	World3D_Update(&world,cam.p,(Vec2){ GetWidth(),GetHeight() });

	for(int i = 0;i<world.trisOut.size;i++){
		Tri3D* t = (Tri3D*)Vector_Get(&world.trisOut,i);
		const Pixel c = Pixel_Mulf(t->c.c,t->c.l);

		if(Mode==0)
			RenderTriangle(((Vec2){ t->p[0].x, t->p[0].y }),((Vec2){ t->p[1].x, t->p[1].y }),((Vec2){ t->p[2].x, t->p[2].y }),c);
		if(Mode==1)
			RenderTriangleWire(((Vec2){ t->p[0].x, t->p[0].y }),((Vec2){ t->p[1].x, t->p[1].y }),((Vec2){ t->p[2].x, t->p[2].y }),c,1.0f);
		if(Mode==2){
			RenderTriangle(((Vec2){ t->p[0].x, t->p[0].y }),((Vec2){ t->p[1].x, t->p[1].y }),((Vec2){ t->p[2].x, t->p[2].y }),c);
			RenderTriangleWire(((Vec2){ t->p[0].x, t->p[0].y }),((Vec2){ t->p[1].x, t->p[1].y }),((Vec2){ t->p[2].x, t->p[2].y }),WHITE,1.0f);
		}
	}

	CStr_RenderAlxFontf(WINDOW_STD_ARGS,GetAlxFont(),0,0,RED,"X: %f, Y: %f, Z: %f",cam.p.x,cam.p.y,cam.p.z);
	CStr_RenderAlxFontf(WINDOW_STD_ARGS,GetAlxFont(),0,window.font.CharSizeY + 1,RED,"SizeIn: %d, SizeBuff: %d, SizeOut: %d",world.trisIn.size,world.trisBuff.size,world.trisOut.size);

	Circle_RenderXWire(WINDOW_STD_ARGS,(Vec2){ GetWidth() / 2,GetHeight() / 2 },5.0f,RED,1.0f);
}
void Delete(AlxWindow* w){
	Vector_Free(&spheres);

	World3D_Free(&world);
	AlxWindow_Mouse_SetVisible(&window);
}

int main(){
	if(Create("Gravity Simulation",2500,1440,1,1,Setup,Update,Delete))
        Start();
    return 0;
}