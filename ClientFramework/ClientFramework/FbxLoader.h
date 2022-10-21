#pragma once
#include "Util.h"
#include "fbxsdk.h"

#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pManager->GetIOSettings()))
#endif

class FbxLoader
{
public:
	void LoadFbxData(vector<Vertex>& vertexVec, vector<UINT>& indexVec, const char* path);

private:
	//fbx씬 로드
	bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);

	//노드를 돌면서 데이터를 넣기위해 재귀함수 호출
	void DisplayContent(FbxNode* pNode, vector<Vertex>& vertexVec, vector<UINT>& indexVec);

	//버텍스벡터와 인덱스벡터에 데이터 파싱해서 넣기
	void DisplayPolygons(FbxMesh* pMesh, vector<Vertex>& vertexVec, vector<UINT>& indexVec);
};