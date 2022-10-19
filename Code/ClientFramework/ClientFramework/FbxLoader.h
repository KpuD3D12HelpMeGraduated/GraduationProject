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
	//fbx�� �ε�
	bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);

	//��带 ���鼭 �����͸� �ֱ����� ����Լ� ȣ��
	void DisplayContent(FbxNode* pNode, vector<Vertex>& vertexVec, vector<UINT>& indexVec);

	//���ؽ����Ϳ� �ε������Ϳ� ������ �Ľ��ؼ� �ֱ�
	void DisplayPolygons(FbxMesh* pMesh, vector<Vertex>& vertexVec, vector<UINT>& indexVec);
};