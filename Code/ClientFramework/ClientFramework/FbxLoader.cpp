#include "FbxLoader.h"

void FbxLoader::LoadFbxData(vector<Vertex>& vertexVec, vector<UINT>& indexVec, const char* path) {
	FbxManager* lSdkManager = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);
	FbxString lPath = FbxGetApplicationDirectory();
	lSdkManager->LoadPluginsDirectory(lPath.Buffer());
	FbxScene* lScene = FbxScene::Create(lSdkManager, "My Scene");
	bool lResult;

	lResult = LoadScene(lSdkManager, lScene, path);
	FbxNode* lNode = lScene->GetRootNode();
	DisplayContent(lNode, vertexVec, indexVec);
}

bool FbxLoader::LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	//int lFileFormat = -1;
	int i, lAnimStackCount;
	bool lStatus;
	char lPassword[1024];

	// Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(pManager, "");

	// Initialize the importer by providing a filename.
	const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)
	{
		FbxString error = lImporter->GetStatus().GetErrorString();
		FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
			FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

		// From this point, it is possible to access animation stack information without
		// the expense of loading the entire file.

		FBXSDK_printf("Animation Stack Information\n");

		lAnimStackCount = lImporter->GetAnimStackCount();

		FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
		FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
		FBXSDK_printf("\n");

		for (i = 0; i < lAnimStackCount; i++)
		{
			FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

			FBXSDK_printf("    Animation Stack %d\n", i);
			FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
			FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

			// Change the value of the import name if the animation stack should be imported 
			// under a different name.
			FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

			// Set the value of the import state to false if the animation stack should be not
			// be imported. 
			FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
			FBXSDK_printf("\n");
		}

		// Set the import states. By default, the import states are always set to 
		// true. The code below shows how to change these states.
		IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
		IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	lStatus = lImporter->Import(pScene);

	if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
	{
		FBXSDK_printf("Please enter password: ");

		lPassword[0] = '\0';

		FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
			scanf("%s", lPassword);
		FBXSDK_CRT_SECURE_NO_WARNING_END

			FbxString lString(lPassword);

		IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
		IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

		lStatus = lImporter->Import(pScene);

		if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
		{
			FBXSDK_printf("\nPassword is wrong, import aborted.\n");
		}
	}

	// Destroy the importer.
	lImporter->Destroy();

	return lStatus;
}

void FbxLoader::DisplayContent(FbxNode* pNode, vector<Vertex>& vertexVec, vector<UINT>& indexVec)
{
	FbxNodeAttribute::EType lAttributeType;
	int j;

	if (pNode->GetNodeAttribute() == NULL)
	{
		FBXSDK_printf("NULL Node Attribute\n\n");
	}
	else
	{
		lAttributeType = (pNode->GetNodeAttribute()->GetAttributeType());

		switch (lAttributeType)
		{
		default:
			break;

		case FbxNodeAttribute::eMesh:
			FbxMesh* lMesh = (FbxMesh*)pNode->GetNodeAttribute();
			DisplayPolygons(lMesh, vertexVec, indexVec);
			break;
		}
	}

	for (j = 0; j < pNode->GetChildCount(); j++)
	{
		DisplayContent(pNode->GetChild(j), vertexVec, indexVec);
	}
}

void FbxLoader::DisplayPolygons(FbxMesh* pMesh, vector<Vertex>& vertexVec, vector<UINT>& indexVec)
{
	int count = pMesh->GetControlPointsCount();
	vertexVec.resize(count);

	FbxVector4* controlPoints = pMesh->GetControlPoints();

	for (int i = 0; i < count; i++)
	{
		vertexVec[i].pos.x = static_cast<float>(controlPoints[i].mData[0]);
		vertexVec[i].pos.y = static_cast<float>(controlPoints[i].mData[2]);
		vertexVec[i].pos.z = static_cast<float>(controlPoints[i].mData[1]);
	}

	int triCount = pMesh->GetPolygonCount();

	indexVec.resize(triCount * 3);

	FbxGeometryElementMaterial* geometryElementMaterial = pMesh->GetElementMaterial();

	const int polygonSize = pMesh->GetPolygonSize(0);
	assert(polygonSize == 3);

	UINT arrIdx[3];
	UINT vertexCounter = 0; //정점의 개수

	for (int i = 0; i < triCount; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			int controlPointIndex = pMesh->GetPolygonVertex(i, j);
			arrIdx[j] = controlPointIndex;

			//Normal 로드
			if (pMesh->GetElementNormalCount() == 0)
				return;

			FbxGeometryElementNormal* normal = pMesh->GetElementNormal();
			UINT normalIdx = 0;

			if (normal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
			{
				if (normal->GetReferenceMode() == FbxGeometryElement::eDirect)
					normalIdx = vertexCounter;
				else
					normalIdx = normal->GetIndexArray().GetAt(vertexCounter);
			}
			else if (normal->GetMappingMode() == FbxGeometryElement::eByControlPoint)
			{
				if (normal->GetReferenceMode() == FbxGeometryElement::eDirect)
					normalIdx = controlPointIndex;
				else
					normalIdx = normal->GetIndexArray().GetAt(controlPointIndex);
			}

			FbxVector4 vec = normal->GetDirectArray().GetAt(normalIdx);
			vertexVec[controlPointIndex].color.x = static_cast<float>(vec.mData[0]);
			vertexVec[controlPointIndex].color.y = static_cast<float>(vec.mData[2]);
			vertexVec[controlPointIndex].color.z = static_cast<float>(vec.mData[1]);

			//UV 로드
			FbxVector2 uv = pMesh->GetElementUV()->GetDirectArray().GetAt(pMesh->GetTextureUVIndex(i, j));
			vertexVec[controlPointIndex].uv.x = static_cast<float>(uv.mData[0]);
			vertexVec[controlPointIndex].uv.y = 1.f - static_cast<float>(uv.mData[1]);

			vertexCounter++;
		}

		indexVec.push_back(arrIdx[0]);
		indexVec.push_back(arrIdx[2]);
		indexVec.push_back(arrIdx[1]);
	}
}