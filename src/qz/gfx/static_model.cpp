#include <qz/gfx/static_texture.hpp>
#include <qz/gfx/static_model.hpp>
#include <qz/gfx/static_mesh.hpp>
#include <qz/gfx/context.hpp>
#include <qz/gfx/assets.hpp>

#include <qz/util/file_view.hpp>
#include <qz/util/macros.hpp>

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <unordered_map>
#include <string>
#include <vector>

namespace qz::gfx {
    template <>
    struct TaskData<StaticModel> {
        meta::Handle<StaticModel> handle;
        const Context* context;
        std::string path;
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normals;
        glm::vec2 uvs;
        glm::vec3 tangents;
        glm::vec3 bitangents;
    };

    qz_nodiscard static meta::Handle<StaticTexture> try_load_texture(const Context& context, const aiMaterial* material, aiTextureType type, std::string_view path) noexcept {
        static std::unordered_map<std::string, meta::Handle<StaticTexture>> cache;
        static std::mutex mutex;

        qz_likely_if(!material->GetTextureCount(type)) {
            return { 0 };
        }
        aiString str;
        material->GetTexture(type, 0, &str);
        const auto file_name = std::string(path) + "/" + str.C_Str();

        std::lock_guard lock(mutex);
        qz_likely_if(cache.contains(file_name)) {
            return cache[file_name];
        }
        return cache[file_name] =
            StaticTexture::request(context, file_name, type == aiTextureType_DIFFUSE ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
    }

    qz_nodiscard static TexturedMesh load_textured_mesh(const Context& context, const aiScene* scene, const aiMesh* mesh, std::string_view path) noexcept {
        std::vector<Vertex> geometry;
        std::vector<std::uint32_t> indices;

        geometry.reserve(mesh->mNumVertices);
        for (std::size_t i = 0; i < mesh->mNumVertices; ++i) {
            auto& vertex = geometry.emplace_back();
            vertex.position[0] = mesh->mVertices[i].x;
            vertex.position[1] = mesh->mVertices[i].y;
            vertex.position[2] = mesh->mVertices[i].z;

            qz_likely_if(mesh->mNormals) {
                vertex.normals[0] = mesh->mNormals[i].x;
                vertex.normals[1] = mesh->mNormals[i].y;
                vertex.normals[2] = mesh->mNormals[i].z;
            }

            qz_likely_if(mesh->mTextureCoords[0]) {
                vertex.uvs[0] = mesh->mTextureCoords[0][i].x;
                vertex.uvs[1] = mesh->mTextureCoords[0][i].y;
            }

            qz_likely_if(mesh->mTangents) {
                vertex.tangents[0] = mesh->mTangents[i].x;
                vertex.tangents[1] = mesh->mTangents[i].y;
                vertex.tangents[2] = mesh->mTangents[i].z;
            }

            qz_likely_if(mesh->mBitangents) {
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
        const auto vertex_size = geometry.size() * (sizeof(Vertex) / sizeof(float));
        const auto index_size = indices.size();
        std::vector<float> vertices(vertex_size);
        std::memcpy(vertices.data(), geometry.data(), sizeof(float) * vertices.size());
        return {
            .mesh = StaticMesh::request(context, { std::move(vertices), std::move(indices) }),
            .diffuse = try_load_texture(context, material, aiTextureType_DIFFUSE, path),
            .normal = try_load_texture(context, material, aiTextureType_HEIGHT, path),
            .spec = try_load_texture(context, material, aiTextureType_SPECULAR, path),
            .vertex_count = vertex_size,
            .index_count = index_size
        };
    }

    static void process_node(const Context& context, const aiScene* scene, const aiNode* node, StaticModel& model, std::string_view path) noexcept {
        for (std::size_t i = 0; i < node->mNumMeshes; i++) {
            model.submeshes.emplace_back(load_textured_mesh(context, scene, scene->mMeshes[node->mMeshes[i]], path));
        }

        for (std::size_t i = 0; i < node->mNumChildren; i++) {
            process_node(context, scene, node->mChildren[i], model, path);
        }
    }

    static void do_model_load(ftl::TaskScheduler*, void* ptr) noexcept {
        const auto* task_data = static_cast<const TaskData<StaticModel>*>(ptr);
        auto& [result, context, path] = *task_data;
        auto scene_file = util::FileView::create(path);
        auto importer = new Assimp::Importer();
        const auto scene = importer->ReadFileFromMemory(scene_file.data(), scene_file.size(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace, path.data());
        util::FileView::destroy(scene_file);
        qz_assert(scene && !scene->mFlags && scene->mRootNode, "failed to load model");
        StaticModel model;
        process_node(*context, scene, scene->mRootNode, model, path.substr(0, path.find_last_of('/')));
        assets::finalize(result, std::move(model));
        context->task_manager->insert({
            .poll = [result = result]() -> VkResult {
                const auto model_lock = assets::acquire<StaticModel>();
                const auto handle     = assets::from_handle(result);
                qz_unlikely_if(std::all_of(handle.submeshes.begin(), handle.submeshes.end(), [](const auto& each) {
                    const auto mesh_lock    = assets::acquire<StaticMesh>();
                    const auto texture_lock = assets::acquire<StaticTexture>();
                    return assets::is_ready(each.mesh)    &&
                           assets::is_ready(each.diffuse) &&
                           assets::is_ready(each.normal)  &&
                           assets::is_ready(each.spec);
                })) {
                    return VK_SUCCESS;
                }
                return VK_NOT_READY;
            },
            .cleanup = [task_data, importer]() {
                delete importer;
                delete task_data;
            }
        });
    }

    qz_nodiscard meta::Handle<StaticModel> StaticModel::request(const Context& context, std::string_view path) noexcept {
        const auto result = assets::emplace_empty<StaticModel>();
        context.task_manager->add_task({
            .Function = do_model_load,
            .ArgData = new TaskData<StaticModel>{
                result,
                &context,
                path.data()
            }
        });
        return result;
    }
} // namespace qz::gfx