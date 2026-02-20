#include "Sprite.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "WinApp.h"
#include "TextureManager.h"

using namespace math;

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
  transformationMatrixData->WVP = math::Math::MakeIdentity4x4();
  transformationMatrixData->World = math::Math::MakeIdentity4x4();

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
  vertexData[0].position = {0.0f, 1.0f, 0.0f, 1.0f};
  vertexData[0].texcoord = {0.0f, 1.0f};

  // 左上
  vertexData[1].position = {0.0f, 0.0f, 0.0f, 1.0f};
  vertexData[1].texcoord = {0.0f, 0.0f};

  // 右下
  vertexData[2].position = {1.0f, 1.0f, 0.0f, 1.0f};
  vertexData[2].texcoord = {1.0f, 1.0f};

  // 右上
  vertexData[3].position = {1.0f, 0.0f, 0.0f, 1.0f};
  vertexData[3].texcoord = {1.0f, 0.0f};

  transform.scale = {size.x, size.y, 1.0f};

  // Sprite用のWorldViewProjectionMatrixを作る
  math::Matrix4x4 worldMatrix = math::Math::MakeAffineMatrix(
      transform.scale, transform.rotate, transform.translate);
  Matrix4x4 viewMatrix = math::Math::MakeIdentity4x4();
  Matrix4x4 projectionMatrix = math::Math::MakeOrthographicMatrix(
      0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight),
      0.0f, 100.0f);
  // Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite,
  // Multiply(viewMatrixSprite, projectionMatrixSprite));
  transformationMatrixData->WVP = math::Math::Multiply(
      worldMatrix, math::Math::Multiply(viewMatrix, projectionMatrix));
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