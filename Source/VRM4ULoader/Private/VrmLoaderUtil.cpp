// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmConvert.h"
#include "VRM4ULoaderLog.h"

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"

#include "Async/ParallelFor.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "PixelFormat.h"
#include "RenderUtils.h"


/////

//test

#if	UE_VERSION_OLDER_THAN(5,0,0)

#if WITH_EDITOR==0
#define VRM4U_UseLocalTGAHeader 1
#endif

#else
#define VRM4U_UseLocalTGAHeader 1
#endif


#ifndef  VRM4U_UseLocalTGAHeader
#define VRM4U_UseLocalTGAHeader 0
#endif

#if	VRM4U_UseLocalTGAHeader

#pragma pack(push,1)

struct FTGAFileHeader
{
	uint8 IdFieldLength;
	uint8 ColorMapType;
	uint8 ImageTypeCode;		// 2 for uncompressed RGB format
	uint16 ColorMapOrigin;
	uint16 ColorMapLength;
	uint8 ColorMapEntrySize;
	uint16 XOrigin;
	uint16 YOrigin;
	uint16 Width;
	uint16 Height;
	uint8 BitsPerPixel;
	uint8 ImageDescriptor;
	friend FArchive& operator<<(FArchive& Ar, FTGAFileHeader& H)
	{
		Ar << H.IdFieldLength << H.ColorMapType << H.ImageTypeCode;
		Ar << H.ColorMapOrigin << H.ColorMapLength << H.ColorMapEntrySize;
		Ar << H.XOrigin << H.YOrigin << H.Width << H.Height << H.BitsPerPixel;
		Ar << H.ImageDescriptor;
		return Ar;
	}
};
#pragma pack(pop)
#endif

void DecompressTGA_RLE_32bpp(const FTGAFileHeader* TGA, uint32* TextureData)
{
	uint8* IdData = (uint8*)TGA + sizeof(FTGAFileHeader);
	uint8* ColorMap = IdData + TGA->IdFieldLength;
	uint8* ImageData = (uint8*)(ColorMap + (TGA->ColorMapEntrySize + 4) / 8 * TGA->ColorMapLength);
	uint32	Pixel = 0;
	int32     RLERun = 0;
	int32     RAWRun = 0;

	for (int32 Y = TGA->Height - 1; Y >= 0; Y--) // Y-flipped.
	{
		for (int32 X = 0; X < TGA->Width; X++)
		{
			if (RLERun > 0)
			{
				RLERun--;  // reuse current Pixel data.
			} else if (RAWRun == 0) // new raw pixel or RLE-run.
			{
				uint8 RLEChunk = *(ImageData++);
				if (RLEChunk & 0x80)
				{
					RLERun = (RLEChunk & 0x7F) + 1;
					RAWRun = 1;
				} else
				{
					RAWRun = (RLEChunk & 0x7F) + 1;
				}
			}
			// Retrieve new pixel data - raw run or single pixel for RLE stretch.
			if (RAWRun > 0)
			{
				Pixel = *(uint32*)ImageData; // RGBA 32-bit dword.
				ImageData += 4;
				RAWRun--;
				RLERun--;
			}
			// Store.
			*((TextureData + Y * TGA->Width) + X) = Pixel;
		}
	}
}

void DecompressTGA_RLE_24bpp(const FTGAFileHeader* TGA, uint32* TextureData)
{
	uint8* IdData = (uint8*)TGA + sizeof(FTGAFileHeader);
	uint8* ColorMap = IdData + TGA->IdFieldLength;
	uint8* ImageData = (uint8*)(ColorMap + (TGA->ColorMapEntrySize + 4) / 8 * TGA->ColorMapLength);
	uint8    Pixel[4] = {};
	int32     RLERun = 0;
	int32     RAWRun = 0;

	for (int32 Y = TGA->Height - 1; Y >= 0; Y--) // Y-flipped.
	{
		for (int32 X = 0; X < TGA->Width; X++)
		{
			if (RLERun > 0)
				RLERun--;  // reuse current Pixel data.
			else if (RAWRun == 0) // new raw pixel or RLE-run.
			{
				uint8 RLEChunk = *(ImageData++);
				if (RLEChunk & 0x80)
				{
					RLERun = (RLEChunk & 0x7F) + 1;
					RAWRun = 1;
				} else
				{
					RAWRun = (RLEChunk & 0x7F) + 1;
				}
			}
			// Retrieve new pixel data - raw run or single pixel for RLE stretch.
			if (RAWRun > 0)
			{
				Pixel[0] = *(ImageData++);
				Pixel[1] = *(ImageData++);
				Pixel[2] = *(ImageData++);
				Pixel[3] = 255;
				RAWRun--;
				RLERun--;
			}
			// Store.
			*((TextureData + Y * TGA->Width) + X) = *(uint32*)&Pixel;
		}
	}
}

void DecompressTGA_RLE_16bpp(const FTGAFileHeader* TGA, uint32* TextureData)
{
	uint8* IdData = (uint8*)TGA + sizeof(FTGAFileHeader);
	uint8* ColorMap = IdData + TGA->IdFieldLength;
	uint16* ImageData = (uint16*)(ColorMap + (TGA->ColorMapEntrySize + 4) / 8 * TGA->ColorMapLength);
	uint16  FilePixel = 0;
	uint32	TexturePixel = 0;
	int32     RLERun = 0;
	int32     RAWRun = 0;

	for (int32 Y = TGA->Height - 1; Y >= 0; Y--) // Y-flipped.
	{
		for (int32 X = 0; X < TGA->Width; X++)
		{
			if (RLERun > 0)
				RLERun--;  // reuse current Pixel data.
			else if (RAWRun == 0) // new raw pixel or RLE-run.
			{
				uint8 RLEChunk = *((uint8*)ImageData);
				ImageData = (uint16*)(((uint8*)ImageData) + 1);
				if (RLEChunk & 0x80)
				{
					RLERun = (RLEChunk & 0x7F) + 1;
					RAWRun = 1;
				} else
				{
					RAWRun = (RLEChunk & 0x7F) + 1;
				}
			}
			// Retrieve new pixel data - raw run or single pixel for RLE stretch.
			if (RAWRun > 0)
			{
				FilePixel = *(ImageData++);
				RAWRun--;
				RLERun--;
			}
			// Convert file format A1R5G5B5 into pixel format B8G8R8B8
			TexturePixel = (FilePixel & 0x001F) << 3;
			TexturePixel |= (FilePixel & 0x03E0) << 6;
			TexturePixel |= (FilePixel & 0x7C00) << 9;
			TexturePixel |= (FilePixel & 0x8000) << 16;
			// Store.
			*((TextureData + Y * TGA->Width) + X) = TexturePixel;
		}
	}
}

void DecompressTGA_32bpp(const FTGAFileHeader* TGA, uint32* TextureData)
{

	uint8* IdData = (uint8*)TGA + sizeof(FTGAFileHeader);
	uint8* ColorMap = IdData + TGA->IdFieldLength;
	uint32* ImageData = (uint32*)(ColorMap + (TGA->ColorMapEntrySize + 4) / 8 * TGA->ColorMapLength);

	for (int32 Y = 0; Y < TGA->Height; Y++)
	{
		FMemory::Memcpy(TextureData + Y * TGA->Width, ImageData + (TGA->Height - Y - 1) * TGA->Width, TGA->Width * 4);
	}
}

void DecompressTGA_16bpp(const FTGAFileHeader* TGA, uint32* TextureData)
{
	uint8* IdData = (uint8*)TGA + sizeof(FTGAFileHeader);
	uint8* ColorMap = IdData + TGA->IdFieldLength;
	uint16* ImageData = (uint16*)(ColorMap + (TGA->ColorMapEntrySize + 4) / 8 * TGA->ColorMapLength);
	uint16    FilePixel = 0;
	uint32	TexturePixel = 0;

	for (int32 Y = TGA->Height - 1; Y >= 0; Y--)
	{
		for (int32 X = 0; X < TGA->Width; X++)
		{
			FilePixel = *ImageData++;
			// Convert file format A1R5G5B5 into pixel format B8G8R8A8
			TexturePixel = (FilePixel & 0x001F) << 3;
			TexturePixel |= (FilePixel & 0x03E0) << 6;
			TexturePixel |= (FilePixel & 0x7C00) << 9;
			TexturePixel |= (FilePixel & 0x8000) << 16;
			// Store.
			*((TextureData + Y * TGA->Width) + X) = TexturePixel;
		}
	}
}

void DecompressTGA_24bpp(const FTGAFileHeader* TGA, uint32* TextureData)
{
	uint8* IdData = (uint8*)TGA + sizeof(FTGAFileHeader);
	uint8* ColorMap = IdData + TGA->IdFieldLength;
	uint8* ImageData = (uint8*)(ColorMap + (TGA->ColorMapEntrySize + 4) / 8 * TGA->ColorMapLength);
	uint8    Pixel[4];

	for (int32 Y = 0; Y < TGA->Height; Y++)
	{
		for (int32 X = 0; X < TGA->Width; X++)
		{
			Pixel[0] = *((ImageData + (TGA->Height - Y - 1) * TGA->Width * 3) + X * 3 + 0);
			Pixel[1] = *((ImageData + (TGA->Height - Y - 1) * TGA->Width * 3) + X * 3 + 1);
			Pixel[2] = *((ImageData + (TGA->Height - Y - 1) * TGA->Width * 3) + X * 3 + 2);
			Pixel[3] = 255;
			*((TextureData + Y * TGA->Width) + X) = *(uint32*)&Pixel;
		}
	}
}

void DecompressTGA_8bpp(const FTGAFileHeader* TGA, uint8* TextureData)
{
	const uint8* const IdData = (uint8*)TGA + sizeof(FTGAFileHeader);
	const uint8* const ColorMap = IdData + TGA->IdFieldLength;
	const uint8* const ImageData = (uint8*)(ColorMap + (TGA->ColorMapEntrySize + 4) / 8 * TGA->ColorMapLength);

	int32 RevY = 0;
	for (int32 Y = TGA->Height - 1; Y >= 0; --Y)
	{
		const uint8* ImageCol = ImageData + (Y * TGA->Width);
		uint8* TextureCol = TextureData + (RevY++ * TGA->Width);
		FMemory::Memcpy(TextureCol, ImageCol, TGA->Width);
	}
}


bool DecompressTGA_helper(
	const int dummy,
	const FTGAFileHeader* TGA,
	uint32*& TextureData,
	const int32 TextureDataSize,
	FFeedbackContext* Warn = nullptr)
{
	if (TGA->ImageTypeCode == 10) // 10 = RLE compressed 
	{
		// RLE compression: CHUNKS: 1 -byte header, high bit 0 = raw, 1 = compressed
		// bits 0-6 are a 7-bit count; count+1 = number of raw pixels following, or rle pixels to be expanded. 
		if (TGA->BitsPerPixel == 32)
		{
			DecompressTGA_RLE_32bpp(TGA, TextureData);
		} else if (TGA->BitsPerPixel == 24)
		{
			DecompressTGA_RLE_24bpp(TGA, TextureData);
		} else if (TGA->BitsPerPixel == 16)
		{
			DecompressTGA_RLE_16bpp(TGA, TextureData);
		} else
		{
			//Warn->Logf(ELogVerbosity::Error, TEXT("TGA uses an unsupported rle-compressed bit-depth: %u"), TGA->BitsPerPixel);
			return false;
		}
	} else if (TGA->ImageTypeCode == 2) // 2 = Uncompressed RGB
	{
		if (TGA->BitsPerPixel == 32)
		{
			DecompressTGA_32bpp(TGA, TextureData);
		} else if (TGA->BitsPerPixel == 16)
		{
			DecompressTGA_16bpp(TGA, TextureData);
		} else if (TGA->BitsPerPixel == 24)
		{
			DecompressTGA_24bpp(TGA, TextureData);
		} else
		{
			//Warn->Logf(ELogVerbosity::Error, TEXT("TGA uses an unsupported bit-depth: %u"), TGA->BitsPerPixel);
			return false;
		}
	}
	// Support for alpha stored as pseudo-color 8-bit TGA
	else if (TGA->ColorMapType == 1 && TGA->ImageTypeCode == 1 && TGA->BitsPerPixel == 8)
	{
		DecompressTGA_8bpp(TGA, (uint8*)TextureData);
	}
	// standard grayscale
	else if (TGA->ColorMapType == 0 && TGA->ImageTypeCode == 3 && TGA->BitsPerPixel == 8)
	{
		DecompressTGA_8bpp(TGA, (uint8*)TextureData);
	} else
	{
		//Warn->Logf(ELogVerbosity::Error, TEXT("TGA is an unsupported type: %u"), TGA->ImageTypeCode);
		return false;
	}

	// Flip the image data if the flip bits are set in the TGA header.
	bool FlipX = (TGA->ImageDescriptor & 0x10) ? 1 : 0;
	bool FlipY = (TGA->ImageDescriptor & 0x20) ? 1 : 0;
	if (FlipY || FlipX)
	{
		TArray<uint8> FlippedData;
		FlippedData.AddUninitialized(TextureDataSize);

		int32 NumBlocksX = TGA->Width;
		int32 NumBlocksY = TGA->Height;
		int32 BlockBytes = TGA->BitsPerPixel == 8 ? 1 : 4;

		uint8* MipData = (uint8*)TextureData;

		for (int32 Y = 0; Y < NumBlocksY; Y++)
		{
			for (int32 X = 0; X < NumBlocksX; X++)
			{
				int32 DestX = FlipX ? (NumBlocksX - X - 1) : X;
				int32 DestY = FlipY ? (NumBlocksY - Y - 1) : Y;
				FMemory::Memcpy(
					&FlippedData[(DestX + DestY * NumBlocksX) * BlockBytes],
					&MipData[(X + Y * NumBlocksX) * BlockBytes],
					BlockBytes
				);
			}
		}
		FMemory::Memcpy(MipData, FlippedData.GetData(), FlippedData.Num());
	}

	return true;
}


bool DecompressTGA(
	const FTGAFileHeader* TGA,
	VRMUtil::FImportImage& OutImage,
	FFeedbackContext* Warn = nullptr)
{
	if (TGA->ColorMapType == 1 && TGA->ImageTypeCode == 1 && TGA->BitsPerPixel == 8)
	{
		// Notes: The Scaleform GFx exporter (dll) strips all font glyphs into a single 8-bit texture.
		// The targa format uses this for a palette index; GFx uses a palette of (i,i,i,i) so the index
		// is also the alpha value.
		//
		// We store the image as PF_G8, where it will be used as alpha in the Glyph shader.
		OutImage.Init2DWithOneMip(
			TGA->Width,
			TGA->Height,
			TSF_G8);
		OutImage.CompressionSettings = TC_Grayscale;
	} else if (TGA->ColorMapType == 0 && TGA->ImageTypeCode == 3 && TGA->BitsPerPixel == 8)
	{
		// standard grayscale images
		OutImage.Init2DWithOneMip(
			TGA->Width,
			TGA->Height,
			TSF_G8);
		OutImage.CompressionSettings = TC_Grayscale;
	} else
	{
		if (TGA->ImageTypeCode == 10) // 10 = RLE compressed 
		{
			if (TGA->BitsPerPixel != 32 &&
				TGA->BitsPerPixel != 24 &&
				TGA->BitsPerPixel != 16)
			{
				//Warn->Logf(ELogVerbosity::Error, TEXT("TGA uses an unsupported rle-compressed bit-depth: %u"), TGA->BitsPerPixel);
				return false;
			}
		} else
		{
			if (TGA->BitsPerPixel != 32 &&
				TGA->BitsPerPixel != 16 &&
				TGA->BitsPerPixel != 24)
			{
				//Warn->Logf(ELogVerbosity::Error, TEXT("TGA uses an unsupported bit-depth: %u"), TGA->BitsPerPixel);
				return false;
			}
		}

		OutImage.Init2DWithOneMip(
			TGA->Width,
			TGA->Height,
			TSF_BGRA8);
	}

	int32 TextureDataSize = OutImage.RawData.Num();
	uint32* TextureData = (uint32*)OutImage.RawData.GetData();

	return DecompressTGA_helper(0, TGA, TextureData, TextureDataSize, Warn);
}


UTexture2D* VRMLoaderUtil::CreateTextureFromImage(FString name, UPackage* package, const void* vBuffer, const size_t Length, bool bGenerateMips, bool bNormal, bool bGreenFlip) {

	const char* Buffer = (const char*)vBuffer;
	VRMUtil::FImportImage img;
	if (VRMLoaderUtil::LoadImageFromMemory(Buffer, Length, img) == false) {
		return nullptr;
	}
	UTexture2D *tex = CreateTexture(img.SizeX, img.SizeY, name, package);

	if (tex == nullptr) {
		return nullptr;
	}
	{
		uint8* MipData = (uint8*)GetPlatformData(tex)->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		if (bGreenFlip) {
			//for (int32 y = 0; y < img.SizeY; y++)
			ParallelFor(img.SizeY, [&](int y) {
				const uint8* c = img.RawData.GetData() + (y * img.SizeX * sizeof(FColor));
				uint8* DestPtr = MipData + (y * img.SizeX * sizeof(FColor));
				for (int32 x = 0; x < img.SizeX; x++)
				{
					DestPtr[0] = c[0];
					DestPtr[1] = 255 - c[1];
					DestPtr[2] = c[2];
					DestPtr[3] = c[3];

					DestPtr += 4;
					c += 4;
				}
				});
		}else{
			FMemory::Memcpy(MipData, img.RawData.GetData(), img.RawData.Num());
		}
			/*
			for (int32 y = 0; y < Height; y++)
			{
				const aiTexel* c = &(t.pcData[y * Width]);
				uint8* DestPtr = &MipData[y * Width * sizeof(FColor)];
				for (int32 x = 0; x < Width; x++)
				{
					*DestPtr++ = c->b;
					*DestPtr++ = c->g;
					*DestPtr++ = c->r;
					*DestPtr++ = c->a;
					c++;
				}
			}
			*/
		GetPlatformData(tex)->Mips[0].BulkData.Unlock();
	}

	// Set options
	tex->SRGB = true;// bUseSRGB;
	tex->CompressionSettings = TC_Default;
	if (bNormal) {
		tex->CompressionSettings = TC_Normalmap;
		tex->SRGB = 0;
#if WITH_EDITORONLY_DATA
		tex->CompressionNoAlpha = true;
#endif
	}
	tex->AddressX = TA_Wrap;
	tex->AddressY = TA_Wrap;

	if (VRMConverter::IsImportMode() == false) {
		// skip updateresource. call manuary
		//tex->UpdateResource();
		return tex;
	}

#if WITH_EDITORONLY_DATA
	tex->CompressionNone = false;
	tex->DeferCompression = true;
	if (bGenerateMips && FMath::IsPowerOfTwo(img.SizeX) && FMath::IsPowerOfTwo(img.SizeY)) {
		tex->MipGenSettings = TMGS_FromTextureGroup;
	} else {
		tex->MipGenSettings = TMGS_NoMipmaps;
	}
	{
		// alpha check
		bool noAlpha = true;
		uint8 *p = img.RawData.GetData();
		for (int y = 0; y < img.SizeY; ++y) {
			for (int x = 0; x < img.SizeX; ++x) {
				if (p[(x + y * img.SizeX) * 4 + 3] != 255) {
					noAlpha = false;
					break;
				}
			}
			if (noAlpha == false) break;
		}
		tex->CompressionNoAlpha = noAlpha;
	}
	tex->Source.Init(img.SizeX, img.SizeY, 1, 1, ETextureSourceFormat::TSF_BGRA8, img.RawData.GetData());
	//NewTexture2D->Source.Compress();
#endif

			// Update the remote texture data
	tex->UpdateResource();
#if WITH_EDITOR
	tex->PostEditChange();
#endif


	return tex;
}


UTexture2D* VRMLoaderUtil::CreateTexture(int32 InSizeX, int32 InSizeY, FString name, UPackage* package) {
	auto format = PF_B8G8R8A8;
	UTexture2D* NewTexture = NULL;
	if (InSizeX > 0 && InSizeY > 0 &&
		(InSizeX % GPixelFormats[format].BlockSizeX) == 0 &&
		(InSizeY % GPixelFormats[format].BlockSizeY) == 0)
	{
		if (package == GetTransientPackage()) {
			NewTexture = NewObject<UTexture2D>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);
		} else {
			TArray<UObject*> ret;
			GetObjectsWithOuter(package, ret);
			for (auto* a : ret) {
				auto s = a->GetName().ToLower();
				if (s.IsEmpty()) continue;

				if (s == name.ToLower()) {
					//a->ClearFlags(EObjectFlags::RF_Standalone);
					//a->SetFlags(EObjectFlags::RF_Public | RF_Transient);
					//a->ConditionalBeginDestroy();
					//a->Rename(TEXT("aaaaaaa"));
					//auto *q = Cast<USkeletalMesh>(a);
					//if (q) {
					//	q->Materials.Empty();
					//}
					a->Rename(NULL, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);

					break;
				}
			}

			NewTexture = VRM4U_NewObject<UTexture2D>(
				// GetTransientPackage(),
				package,
				*name,
				//RF_Transient
				RF_Public | RF_Standalone
				);
		}
		NewTexture->Modify();
#if WITH_EDITOR
		NewTexture->PreEditChange(NULL);
#endif

		SetPlatformData(NewTexture, new FTexturePlatformData());
		GetPlatformData(NewTexture)->SizeX = InSizeX;
		GetPlatformData(NewTexture)->SizeY = InSizeY;
		GetPlatformData(NewTexture)->PixelFormat = format;

		int32 NumBlocksX = InSizeX / GPixelFormats[format].BlockSizeX;
		int32 NumBlocksY = InSizeY / GPixelFormats[format].BlockSizeY;
#if	UE_VERSION_OLDER_THAN(4,23,0)
		FTexture2DMipMap* Mip = new(NewTexture->PlatformData->Mips) FTexture2DMipMap();
#else
		FTexture2DMipMap* Mip = new FTexture2DMipMap();
		GetPlatformData(NewTexture)->Mips.Add(Mip);
#endif
		Mip->SizeX = InSizeX;
		Mip->SizeY = InSizeY;
		Mip->BulkData.Lock(LOCK_READ_WRITE);
		Mip->BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[format].BlockBytes);
		Mip->BulkData.Unlock();
	} else
	{
		UE_LOG(LogVRM4ULoader, Warning, TEXT("Invalid parameters specified for UTexture2D::Create()"));
	}
	return NewTexture;
}



bool VRMLoaderUtil::LoadImageFromMemory(const void* vBuffer, const size_t Length, VRMUtil::FImportImage& OutImage) {
	const char* Buffer = (const char*)vBuffer;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TArray<TSharedPtr<IImageWrapper> > ImageWrapperList = {
		ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG),
		ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG),
		ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP),
	};

	TSharedPtr<IImageWrapper> ImageWrapper;
	for (auto& a : ImageWrapperList) {
		if (a.IsValid() == false) continue;
		if (a->SetCompressed(Buffer, Length) == false) continue;
		ImageWrapper = a;
		break;
	}
	if (ImageWrapper.IsValid()) {

#if	UE_VERSION_OLDER_THAN(4,25,0)
#else
		TArray<uint8> RawData;
#endif
		const TArray<uint8>* pRawData = nullptr;

#if	UE_VERSION_OLDER_THAN(4,25,0)
		if (ImageWrapper.IsValid()) {
			if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, pRawData) == false) return false;
		}
#else
		if (ImageWrapper.IsValid()) {
			if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData) == false) return false;
		}
		pRawData = &RawData;
#endif

		const int Width = FMath::Max(ImageWrapper->GetWidth(), 1);
		const int Height = FMath::Max(ImageWrapper->GetHeight(), 1);
		OutImage.Init2DWithOneMip(Width, Height, TSF_BGRA8);

		{
			uint8* MipData = (uint8*)OutImage.RawData.GetData();
			if (ImageWrapper.IsValid()) {
				if (pRawData) {
					FMemory::Memcpy(MipData, pRawData->GetData(), pRawData->Num());
				} else {
					for (int32 y = 0; y < Height; y++)
					{
						const uint8* c = pRawData->GetData() + (y * Width);
						uint8* DestPtr = &MipData[y * Width * sizeof(FColor)];
						for (int32 x = 0; x < Width; x++)
						{
							*DestPtr++ = c[0];
							*DestPtr++ = c[1];
							*DestPtr++ = c[2];
							*DestPtr++ = c[3];
							c++;
						}
					}
				}
			} else {
				//int no = pmxTexNameList.Num();
				//MipData[0] = (no & 1 ? 255 : 0);
				//MipData[1] = (no & 2 ? 255 : 0);
				//MipData[2] = (no & 4 ? 255 : 0);
				//MipData[3] = 255;
			}
		}
		return true;
	}


	//
	// TGA
	//
	// Support for alpha stored as pseudo-color 8-bit TGA
	const FTGAFileHeader* TGA = (FTGAFileHeader*)Buffer;
	if (Length >= sizeof(FTGAFileHeader) &&
		((TGA->ColorMapType == 0 && TGA->ImageTypeCode == 2) ||
			// ImageTypeCode 3 is greyscale
			(TGA->ColorMapType == 0 && TGA->ImageTypeCode == 3) ||
			(TGA->ColorMapType == 0 && TGA->ImageTypeCode == 10) ||
			(TGA->ColorMapType == 1 && TGA->ImageTypeCode == 1 && TGA->BitsPerPixel == 8)))
	{
		// Check the resolution of the imported texture to ensure validity
		//if (!IsImportResolutionValid(TGA->Width, TGA->Height, bAllowNonPowerOfTwo, Warn))
		//{
		//	return false;
		//}

		const bool bResult = DecompressTGA(TGA, OutImage);
		if (bResult && OutImage.CompressionSettings == TC_Grayscale && TGA->ImageTypeCode == 3)
		{
			// default grayscales to linear as they wont get compression otherwise and are commonly used as masks
			OutImage.SRGB = false;
		}

		return bResult;
	}

	return false;
}



