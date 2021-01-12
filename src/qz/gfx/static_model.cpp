#include <qz/gfx/static_texture.hpp>
#include <qz/gfx/static_model.hpp>
#include <qz/gfx/static_mesh.hpp>
#include <qz/gfx/assets.hpp>

#include <qz/util/file_view.hpp>
#include <qz/util/macros.hpp>

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <unordered_map>

namespace qz::gfx {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normals;
        glm::vec2 uvs;
        glm::vec3 tangents;
        glm::vec3 bitangents;
    };

    static std::unordered_map<std::string, meta::Handle<StaticTexture>> cache;

    qz_nodiscard static meta::Handle<StaticTexture> try_load_texture(const Context& context, const aiMaterial* material, aiTextureType type, std::string_view path) noexcept {
        if (!material->GetTextureCount(type)) {
            return { 0 };
        }
        aiString str;
        material->GetTexture(type, 0, &str);
        const auto file_name = std::string(path) + "/" + str.C_Str();
        if (cache.contains(file_name)) {
            return cache[file_name];
        }
        return cache[file_name] =
            StaticTexture::request(context, file_name, type == aiTextureType_DIFFUSE ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
    }

    qz_nodiscard static TexturedMesh load_textured_mesh(const Context& context, const aiScene* scene, const aiMesh* mesh, std::string_view path) noexcept {
        TexturedMesh sub_mesh{};
        std::vector<Vertex> geometry;
        std::vector<std::uint32_t> indices;

        geometry.reserve(mesh->mNumVertices);
        for (std::size_t i = 0; i < mesh->mNumVertices; ++i) {
            auto& vertex = geometry.emplace_back();
            vertex.position[0] = mesh->mVertices[i].x;
            vertex.position[1] = mesh->mVertices[i].y;
            vertex.position[2] = mesh->mVertices[i].z;

            vertex.normals[0] = mesh->mNormals[i].x;
            vertex.normals[1] = mesh->mNormals[i].y;
            vertex.normals[2] = mesh->mNormals[i].z;

            if (mesh->mTextureCoords[0]) {
                vertex.uvs[0] = mesh->mTextureCoords[0][i].x;
                vertex.uvs[1] = mesh->mTextureCoords[0][i].y;
            }

            if (mesh->mTangents) {
                vertex.tangents[0] = mesh->mTangents[i].x;
                vertex.tangents[1] = mesh->mTangents[i].y;
                vertex.tangents[2] = mesh->mTangents[i].z;
            }

            if (mesh->mBitangents) {
                vertex.bitangents[0] = mesh->mBitangents[i].x;
                vertex.bitangents[1] = mesh->mBitangents[i].y;
                vertex.bitangents[2] = mesh->mBitangents[i].z;
            }
        }

        indices.reserve(mesh->mNumFaces * 3);
        for (std::size_t i = 0; i < mesh->mNumFaces; i++) {
            auto& face = mesh->mFaces[i];
            for (std::size_t j = 0; j < face.mNumIndices; j++) {
                indices.emplace_back(face.mIndices[j]);
            }
        }

        const auto material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<float> vertices(geometry.size() * (sizeof(Vertex) / sizeof(float)));
        std::memcpy(vertices.data(), geometry.data(), sizeof(float) * vertices.size());
        sub_mesh.vertex_count = vertices.size();
        sub_mesh.index_count = indices.size();
        sub_mesh.mesh = StaticMesh::request(context, { vertices, indices });
        sub_mesh.diffuse = try_load_texture(context, material, aiTextureType_DIFFUSE, path);
        sub_mesh.normal = try_load_texture(context, material, aiTextureType_HEIGHT, path);
        sub_mesh.spec = try_load_texture(context, material, aiTextureType_SPECULAR, path);

        return sub_mesh;
    }

    static void process_node(const Context& context, const aiScene* scene, const aiNode* node, StaticModel& model, std::string_view path) noexcept {
        for (std::size_t i = 0; i < node->mNumMeshes; i++) {
            auto mesh = scene->mMeshes[node->mMeshes[i]];
            model.submeshes.emplace_back(load_textured_mesh(context, scene, mesh, path));
        }

        for (std::size_t i = 0; i < node->mNumChildren; i++) {
            process_node(context, scene, node->mChildren[i], model, path);
        }
    }

    qz_nodiscard StaticModel StaticModel::request(const Context& context, std::string_view path) noexcept {
        StaticModel model{};
        Assimp::Importer importer{};
        //auto scene_file = util::FileView::create(path);
        const auto scene = importer.ReadFile(path.data(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
        qz_assert(scene && !scene->mFlags && scene->mRootNode, "failed to load model");
        process_node(context, scene, scene->mRootNode, model, path.substr(0, path.find_last_of('/')));
        //util::FileView::destroy(scene_file);
        return model;
    }
} // namespace qz::gfx