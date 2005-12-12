
#include "./metalhandler.h"
#include "string.h"
#include "vector"
#include "list"
#include "hash_map"
#include "iterator"
#include "./metalspotmap.h"

#include "../rts/unitdef.h"


#define SAVEGAME_VER 4
MetalSpotMap mdp;

/** MAGIC NUMBERS **/

#define MIN_HOTSPOT_METAL			64
#define MIN_PATCH_METAL				4
#define SEARCH_PASS					64
#define HOT_SPOT_RADIUS_MULTIPLYER	1.0f
#define NEAR_RADIUS			600
CMetalHandler::CMetalHandler(IAICallback *callback)
{
	cb=callback;
	mdp.Initialize(callback);
}
CMetalHandler::~CMetalHandler(void)
{
};


bool metalcmp(float3 a, float3 b) { return a.y > b.y; }








std::vector<float3> *CMetalHandler::parseMap(){
	std::vector<float3> *ov = new std::vector<float3>;
	std::list<float3> metmap = mdp.metalpatch;
	for(list<float3>::iterator rmp = metmap.begin(); rmp !=metmap.end();++rmp){
		ov->push_back(*rmp);
	}
	return ov;
};


void CMetalHandler::saveState() {
	FILE *fp=fopen(hashname(),"w");
	
	int v=SAVEGAME_VER;

	fwrite(&v,sizeof(int),1,fp);

	int s=(int)metalpatch.size();
	fwrite(&s,sizeof(int),1,fp);
	for(int i=0;i<s;i++) {
		fwrite(&metalpatch[i],sizeof(float3),1,fp);
	}
	s=(int)hotspot.size();
	fwrite(&s,sizeof(int),1,fp);
	for(int i=0;i<s;i++) {
		fwrite(&hotspot[i],sizeof(float3),1,fp);
	}
	fclose(fp);

}

void CMetalHandler::loadState() {
	FILE *fp=NULL;
	//Can't get saving working.. bah.
	//fp=fopen(hashname(),"r");
	metalpatch.clear();
	//hotspot.clear();
	if (fp!=NULL) {
		int s=0;
		fread(&s,sizeof(int),1,fp);
		if (s!=SAVEGAME_VER) {
			cb->SendTextMsg("Wrong Savegame Magic",0);
			fclose(fp);
			fp=NULL;
		}
	}
	if (fp==NULL) {
		
		
		std::vector<float3> *vi=parseMap();


		for (std::vector<float3>::iterator it=vi->begin();it!=vi->end();++it) {
		


			metalpatch.push_back(*it);
		
		}

		delete(vi);
		float sr=cb->GetExtractorRadius()*HOT_SPOT_RADIUS_MULTIPLYER;
		if (sr<256) sr=256;
		sr=sr*sr;

		for (unsigned int i=0;i<this->metalpatch.size();i++){
		float3 candidate=metalpatch[i];
		bool used=false;
	
		for (unsigned int j=0;j<hotspot.size();j++) {
			float3 t=hotspot[j];
			if (((candidate.x-t.x)*(candidate.x-t.x)+(candidate.z-t.z)*(candidate.z-t.z))<sr){
				used=true;
				hotspot[j]=float3((t.x*t.y+candidate.x*candidate.y)/(t.y+candidate.y),t.y+candidate.y,(t.z*t.y+candidate.z*candidate.y)/(t.y+candidate.y));
				
			}
		}
		if (used==false) {
			if (candidate.y>MIN_HOTSPOT_METAL)
			hotspot.push_back(candidate);

		}


	}

		saveState();
	cb->SendTextMsg("Generated Metal Info File",0);

	} else {

	
		int s;
	fread(&s,sizeof(int),1,fp);
	for(int i=0;i<s;i++) {
		float3 tmp;
		fread(&tmp,sizeof(float3),1,fp);
		metalpatch.push_back(tmp);
	}
	fread(&s,sizeof(int),1,fp);
	for(int i=0;i<s;i++) {
		float3 tmp;
		fread(&tmp,sizeof(float3),1,fp);
		hotspot.push_back(tmp);
	}
	cb->SendTextMsg("Readed Metal Info File",0);
	fclose(fp);
	}


}


const char *CMetalHandler::hashname() {
	
	const char *name=cb->GetMapName();
	

	std::string *hash=new std::string("./aidll/globalai/");

	hash->append(cb->GetMapName());
	
	hash->append(".metalInfo");
	

	return hash->c_str();

}



float CMetalHandler::getExtractionRanged(float x, float z) {
	float x1=x/16.0f;
	float z1=z/16.0f;


	
	float rad=cb->GetExtractorRadius()/16.0f;

	rad=rad*0.866f;
	float count=0.0f;
	float total=0.0f;

	for (float i=-rad;i<rad;i++){
		for (float j=-rad;j<rad;j++){
			total+=getMetalAmount((int)(x1+i),(int)(z1+j));
			++count;
		}

	}
	if (count==0) return 0.0;

	float ret=total/count;
	
	return ret;
}



float CMetalHandler::getMetalAmount(int x, int z) 
{
	int sizex=(int)(cb->GetMapWidth()/2.0);
	int sizez=(int)(cb->GetMapHeight()/2.0);
	
	
	if(x<0)
		x=0;
	else if(x>=sizex)
		x=sizex-1;

	if(z<0)
		z=0;
	else if(z>=sizez)
		z=sizez-1;



	return (float)(cb->GetMetalMap()[x + z*sizex]);
}


std::vector<float3> *CMetalHandler::getHotSpots() {

	
	return &(this->hotspot);
}



std::vector<float3> *CMetalHandler::getMetalPatch(float3 pos, float minMetal,float radius, float depth){

	std::vector<float3> *v=new std::vector<float3>();
	unsigned int i;
	for (i=0;i<this->metalpatch.size();i++) {
		float3 t1=metalpatch[i];
		if (t1.y<minMetal) break;

		float d=(pos.x-t1.x)*(pos.x-t1.x)+(pos.z-t1.z)*(pos.z-t1.z)-radius*radius;
		for (std::set<int>::iterator it=mex.begin();it!=mex.end();++it) {
			float3 t2=cb->GetUnitPos(*it);
			
			float d1=(t1.x-t2.x)*(t1.x-t2.x)+(t1.z-t2.z)*(t1.z-t2.z);
			//allows for a little overlap
			if (d1<radius*radius*0.866*0.866) {
				if (depth<=cb->GetUnitDef(*it)->extractsMetal) {
					d=-1;
				}
			}
		}
		if (d<0) {
			v->push_back(t1);

		}
	}

	return v;
}

float3 CMetalHandler::getNearestPatch(float3 pos, float minMetal, float depth) {
	
	std::vector<float3> *v=getMetalPatch(pos,minMetal,NEAR_RADIUS,depth);

	if(v->size()==0){
	delete(v); 
	return float3(0,-1,0);	
	}
	float3 vpos=(*v)[0];
	
	delete(v);

	return vpos;	
}
