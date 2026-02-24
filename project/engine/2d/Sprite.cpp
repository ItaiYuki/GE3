#include "Sprite.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "WinApp.h"
#include "TextureManager.h"

using namespace MyMath;

void Sprite::Initialize(SpriteCommon *spriteCommon,
                        std::string textureFilePath) {
  // 引数で受け取ってメンバ変数に記録する
  this->spriteCommon_ = spriteCommon;

  vertexResource = spriteCommon_->GetDxCommon()->CreateBufferResource(
      sizeof(VertexData) * 4);
  indexResource =
      spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * 6);

  // リソースの先頭のアドレスから使う
  vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
  // 使用するリソースのサイズは頂点６つ分のサイズ
  vertexBufferView.SizeInBytes = sizeof(VertexData) * 4;
  // １頂点あたりのサイズ
  vertexBufferView.StrideInBytes = sizeof(VertexData);

  // リソースの先頭のアドレスから使う
  indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
  // 使用するリソースのサイズはインデックス６つ分のサイズ
  indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
  // インデックスはuint32_tとする
  indexBufferView.Format = DXGI_FORMAT_R32_UINT;

  // 書き込むためのアドレスを取得
  vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

  vertexData[0].position = {0.0f, 360.0f, 0.0f, 1.0f};
  vertexData[0].texcoord = {0.0f, 1.0f};

  vertexData[1].position = {0.0f, 0.0f, 0.0f, 1.0f};
  vertexData[1].texcoord = {0.0f, 0.0f};

  vertexData[2].position = {640.0f, 360.0f, 0.0f, 1.0f};
  vertexData[2].texcoord = {1.0f, 1.0f};

  vertexData[3].position = {640.0f, 0.0f, 0.0f, 1.0f};
  vertexData[3].texcoord = {1.0f, 0.0f};

  indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
  indexData[0] = 0;
  indexData[1] = 1;
  indexData[2] = 2;
  indexData[3] = 1;
  indexData[4] = 3;
  indexData[5] = 2;

  // マテリアル用のリソースを作る。今回はcolor１つ分のサイズを用意する
  materialResource =
      spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
  // 書き込むためのアドレスを取得
  materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
  // 今回は白を書き込んでみる
  materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

  // Sparite用のTransformationMatrix用のリソースを作る。Matrix4x4
  // １つ分のサイズを用意する
  transformationMatrixResource =
      spriteCommon_->GetDxCommon()->CreateBufferResource(
          sizeof(TransformationMatrix));
  // 書き込むためのアドレスを取得
  transformationMatrixResource->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData));
  // 単位行列を書きこんでおく
  transformationMatrixData->WVP = MyMath::Math::MakeIdentity4x4();
  transformationMatrixData->World = MyMath::Math::MakeIdentity4x4();

  transform = {};
  transform.scale = {1.0f, 1.0f, 1.0f};

  // テクスチャ番号を取得して記録
  textureIndex =
      TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
}

void Sprite::Update() {

  // 座標
  transform.translate = {position.x, position.y, 0.0f};
  // 回転
  transform.rotate = {0.0f, 0.0f, rotation};

  float left = 0.0f - anchorPoint.x;
  float right = 1.0f - anchorPoint.x;
  float top = 0.0f - anchorPoint.y;
  float bottom = 1.0f - anchorPoint.y;

  // 左右反転
  if (isFlipX_) {
    left = -left;
    right = -right;
  }

  // 上下反転
  if (isFlipY_) {
    top = -top;
    bottom = -bottom;
  }

  // テクスチャ範囲指定
  const DirectX::TexMetadata &metadata =
      TextureManager::GetInstance()->GetMetaData(textureIndex);
  float tex_left = textureLeftTop.x / metadata.width;
  float tex_right = (textureLeftTop.x + textureSize.x) / metadata.width;
  float tex_top = textureLeftTop.y / metadata.height;
  float tex_bottom = (textureLeftTop.y + textureSize.y) / metadata.height;


  // サイズ
  //  頂点リソースにデータを書き込む
  // 左下
  vertexData[0].position = {left, bottom, 0.0f, 1.0f};
  vertexData[0].texcoord = {tex_left, tex_bottom};

  // 左上
  vertexData[1].position = {left, top, 0.0f, 1.0f};
  vertexData[1].texcoord = {tex_left, tex_top};

  // 右下
  vertexData[2].position = {right, bottom, 0.0f, 1.0f};
  vertexData[2].texcoord = {tex_right, tex_bottom};

  // 右上
  vertexData[3].position = {right, top, 0.0f, 1.0f};
  vertexData[3].texcoord = {tex_right, tex_top};

  transform.scale = {size.x, size.y, 1.0f};

  // Sprite用のWorldViewProjectionMatrixを作る
  MyMath::Matrix4x4 worldMatrix = MyMath::Math::MakeAffineMatrix(
      transform.scale, transform.rotate, transform.translate);
  Matrix4x4 viewMatrix = MyMath::Math::MakeIdentity4x4();
  Matrix4x4 projectionMatrix = MyMath::Math::MakeOrthographicMatrix(
      0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight),
      0.0f, 100.0f);
  // Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite,
  // Multiply(viewMatrixSprite, projectionMatrixSprite));
  transformationMatrixData->WVP = MyMath::Math::Multiply(
      worldMatrix, MyMath::Math::Multiply(viewMatrix, projectionMatrix));
  transformationMatrixData->World = worldMatrix;
}

void Sprite::Draw() {
  // Spriteの描画。変更が必要なものだけ変更
  spriteCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(
      0, 1, &vertexBufferView);
  spriteCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(
      &indexBufferView);
  // TransformationMatrixCBufferの場所を設定
  spriteCommon_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          0, transformationMatrixResource->GetGPUVirtualAddress());
  spriteCommon_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          1, materialResource->GetGPUVirtualAddress());
  spriteCommon_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex));
  // 描画！（DrawInstanced(DrawCall/ドローコル）
  // spriteCommon_->GetDxCommon()->GetCommandList()->DrawInstanced(6, 1, 0, 0);
  ////描画!(DrawCall/ドローコル）６個のインデックスを使用し１つのインスタンスを描画。その他は当面０で良い
  spriteCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(6, 1, 0,
                                                                       0, 0);
}

void Sprite::AbjustTextureSize() {
  // テクスチャメタデータ取得
  const DirectX::TexMetadata &metadata =
      TextureManager::GetInstance()->GetMetaData(textureIndex);

  textureSize.x = static_cast<float>(metadata.width);
  textureSize.y = static_cast<float>(metadata.height);
  // 画像サイズをテクスチャサイズに合わせる
  size = textureSize;
    
    }
