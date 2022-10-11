#include "Device.h"
#include "CmdQueue.h"
#include "Texture.h"

void Texture::CreateTexture(const wstring& path, shared_ptr<Device> devicePtr, shared_ptr<CmdQueue> cmdQueuePtr)
{
	wstring ext = fs::path(path).extension();

	if (ext == L".dds" || ext == L".DDS") //dds
		LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, nullptr, _image);
	else if (ext == L".tga" || ext == L".TGA") //tga
		LoadFromTGAFile(path.c_str(), nullptr, _image);
	else //png, jpg, jpeg, bmp
		LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, nullptr, _image);

	HRESULT hr = ::CreateTexture(devicePtr->_device.Get(), _image.GetMetadata(), &_tex2D);
	assert(SUCCEEDED(hr));

	vector<D3D12_SUBRESOURCE_DATA> subResources;

	hr = ::PrepareUpload(devicePtr->_device.Get(), _image.GetImages(), _image.GetImageCount(), _image.GetMetadata(), subResources);

	assert(SUCCEEDED(hr));

	const UINT bufferSize = ::GetRequiredIntermediateSize(_tex2D.Get(), 0, static_cast<UINT>(subResources.size()));

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	ComPtr<ID3D12Resource> textureUploadHeap;
	hr = devicePtr->_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(textureUploadHeap.GetAddressOf()));

	assert(SUCCEEDED(hr));

	UpdateSubresources(cmdQueuePtr->_resCmdList.Get(), _tex2D.Get(), textureUploadHeap.Get(), 0, 0, static_cast<unsigned int>(subResources.size()), subResources.data());

	cmdQueuePtr->FlushResourceCommandQueue();
}

void Texture::CreateSRV(shared_ptr<Device> devicePtr)
{
	//SRVHeap 서술
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	devicePtr->_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_srvHeap));

	_srvHandle = _srvHeap->GetCPUDescriptorHandleForHeapStart();

	//SRV 서술
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = _image.GetMetadata().format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	devicePtr->_device->CreateShaderResourceView(_tex2D.Get(), &srvDesc, _srvHandle);
}