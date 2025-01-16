/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 10 авг. 2021 г.
 *
 * lsp-dsp-units is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-dsp-units is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-dsp-units. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_RAYTRACE3D_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_RAYTRACE3D_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/3d/rt/types.h>
#include <lsp-plug.in/dsp-units/3d/rt/context.h>
#include <lsp-plug.in/dsp-units/3d/raytrace.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/darray.h>

namespace lsp
{
    namespace dspu
    {
        /** Ray tracing storage implemented as a stack
         *
         */
        class LSP_DSP_UNITS_PUBLIC RayTrace3D
        {
            protected:
                typedef struct sample_t
                {
                    Sample             *sample;
                    size_t              channel;
                    ssize_t             r_min;
                    ssize_t             r_max;
                } sample_t;

                typedef struct rt_binding_t
                {
                    lltl::darray<sample_t>          bindings;       // Capture bindings
                } rt_binding_t;

                typedef struct capture_t: public rt_capture_settings_t
                {
                    dsp::vector3d_t                 direction;      // Direction
                    dsp::bound_box3d_t              bbox;           // Bounding box
                    lltl::darray<rt::triangle_t>    mesh;           // Mesh associated with capture
                    lltl::darray<sample_t>          bindings;       // Capture bindings
                } capture_t;

                typedef struct rt_object_t
                {
                    dsp::bound_box3d_t              bbox;
                    lltl::darray<rtx::triangle_t>   mesh;
                    lltl::darray<rtx::edge_t>       plan;
                } rt_object_t;

                typedef struct stats_t
                {
                    uint64_t            root_tasks;
                    uint64_t            local_tasks;
                    uint64_t            calls_scan;
                    uint64_t            calls_cull;
                    uint64_t            calls_split;
                    uint64_t            calls_cullback;
                    uint64_t            calls_reflect;
                    uint64_t            calls_capture;
                } stats_t;

            protected:
                class TaskThread: public ipc::Thread
                {
                    private:
                        RayTrace3D                     *trace;
                        stats_t                         stats;
                        ssize_t                         heavy_state;
                        lltl::parray<rt::context_t>     tasks;
                        lltl::parray<rt_binding_t>      bindings;       // Bindings
                        lltl::parray<rt_object_t>       objects;

                    protected:
                        status_t    main_loop();
                        status_t    process_context(rt::context_t *ctx);

                        status_t    copy_objects(lltl::parray<rt_object_t> *src);
                        status_t    scan_objects(rt::context_t *ctx);
                        status_t    cull_view(rt::context_t *ctx);
                        status_t    split_view(rt::context_t *ctx);
                        status_t    cullback_view(rt::context_t *ctx);
                        status_t    reflect_view(rt::context_t *ctx);
                        status_t    capture(capture_t *capture, lltl::darray<sample_t> *bindings, const rt::view_t *v);

                        status_t    generate_root_mesh();
                        status_t    generate_capture_mesh(size_t id, capture_t *c);
                        status_t    generate_object_mesh(ssize_t id, rt_object_t *o, rt::mesh_t *src, Object3D *obj, const dsp::matrix3d_t *m);
                        status_t    generate_tasks(lltl::parray<rt::context_t> *tasks, float initial);
                        status_t    check_object(rt::context_t *ctx, Object3D *obj, const dsp::matrix3d_t *m);

                        status_t    submit_task(rt::context_t *ctx);

                    public:
                        explicit TaskThread(RayTrace3D *trace);
                        virtual ~TaskThread();

                    public:
                        status_t    prepare_main_loop(float initial);
                        status_t    prepare_captures();
                        status_t    prepare_supplementary_loop(TaskThread *t);

                        virtual status_t run();

                        status_t    merge_result();

                        inline stats_t *get_stats() { return &stats; }
                };

            private:
                lltl::darray<rt::material_t>        vMaterials;
                lltl::darray<rt_source_settings_t>  vSources;
                lltl::parray<capture_t>             vCaptures;
                Scene3D                            *pScene;
                rt::progress_func_t                 pProgress;
                void                               *pProgressData;
                size_t                              nSampleRate;
                float                               fEnergyThresh;
                float                               fTolerance;
                float                               fDetalization;
                bool                                bNormalize;
                volatile bool                       bCancelled;
                volatile bool                       bFailed;

                lltl::parray<rt::context_t>         vTasks;
                size_t                              nQueueSize;
                size_t                              nProgressPoints;
                size_t                              nProgressMax;
                ipc::Mutex                          lkTasks;

            protected:
                static void destroy_tasks(lltl::parray<rt::context_t> *tasks);
                static void destroy_objects(lltl::parray<rt_object_t> *objects);
                static void clear_stats(stats_t *stats);
                static void dump_stats(const char *label, const stats_t *stats);
                static void merge_stats(stats_t *dst, const stats_t *src);

                static bool check_bound_box(const dsp::bound_box3d_t *bbox, const rt::view_t *view);

                void        remove_scene(bool destroy);
                status_t    resize_materials(size_t objects);

                status_t    report_progress(float progress);

                // Main ray-tracing routines
                void        normalize_output();
                bool        is_already_passed(const sample_t *bind);

                status_t    do_process(size_t threads, float initial);

            public:
                explicit RayTrace3D();
                RayTrace3D(const RayTrace3D &) = delete;
                RayTrace3D(RayTrace3D &&) = delete;
                ~RayTrace3D();

                RayTrace3D & operator = (const RayTrace3D &) = delete;
                RayTrace3D & operator = (RayTrace3D &&) = delete;

            public:
                /**
                 * Initialize raytrace object
                 */
                status_t init();

                /** Destroy the ray tracing processor state
                 * @param recursive destroy the currently used scene object
                 */
                void destroy(bool recursive=false);

                /**
                 * Set scene object
                 * @param scene scene object to set
                 * @param destroy destroy previously used scene
                 * @return status of operation
                 */
                status_t set_scene(Scene3D *scene, bool destroy=true);

                /**
                 * Set/clear progress callback
                 * @param callback callback routine to report progress
                 * @param data data that will be passed to callback routine
                 * @return status of operation
                 */
                status_t set_progress_callback(rt::progress_func_t callback, void *data);

                /**
                 * Clear progress callback
                 * @return status of operation
                 */
                status_t clear_progress_callback();

                /**
                 * Set the material for the corresponding object
                 * @param idx the index of the material
                 * @return status of operation
                 */
                status_t    set_material(size_t idx, const rt::material_t *material);

                /**
                 * Get the material for the corresponding object
                 * @param material pointer to store materisl
                 * @param idx the index of the corresponding object
                 * @return pointer to material or NULL
                 */
                status_t    get_material(rt::material_t *material, size_t idx);

                /**
                 * Get the scene object
                 * @param idx scene object
                 * @return object pointer or NULL
                 */
                inline Object3D *object(size_t idx) { return (pScene != NULL) ? pScene->get_object(idx) : NULL; }

                /**
                 * Set sample rate
                 * @param sr sample rate
                 */
                inline void set_sample_rate(size_t sr) { nSampleRate = sr; }

                /**
                 * Get sample rate
                 * @return sample rate
                 */
                inline size_t get_sample_rate() const { return nSampleRate; }

                /**
                 * Add audio source
                 * @param settings source settings
                 * @return status of operation
                 */
                status_t add_source(const rt_source_settings_t *settings);

                /**
                 * Add audio capture
                 * @param settings capture settings
                 * @return non-negative capture identifier or negative error status code
                 */
                ssize_t add_capture(const rt_capture_settings_t *settings);

                /**
                 * Bind audio sample to capture
                 * @param id capture identifier
                 * @param sample audio sample to bind
                 * @param channel number of channel of the sample that will be modified by capture
                 * @param r_min the minimum reflection index, negative value for any
                 * @param r_max the maximum reflection index, negative value for any
                 * @return status of operation
                 */
                status_t    bind_capture(size_t id, Sample *sample, size_t channel, ssize_t r_min, ssize_t r_max);

                /** Remove all audio sources
                 *
                 */
                inline void         clear_sources()     {   vSources.flush();       };

                /** Remove all audio captures
                 *
                 */
                inline void         clear_captures()    {   vCaptures.flush();      };

                inline float        get_energy_threshold() const { return fEnergyThresh; }

                void                set_energy_threshold(float thresh) { fEnergyThresh = thresh; }

                inline float        get_tolerance() const { return fTolerance; }
                inline float        get_detalization() const { return fDetalization; }

                void                set_tolerance(float tolerance) { fTolerance = tolerance; }
                void                set_detalization(float detail) { fDetalization = detail; }

                inline bool         get_normalize() const { return bNormalize; };
                inline void         set_normalize(bool normalize) { bNormalize = normalize; };

                /**
                 * This method indicates that the cancel request was sent to
                 * the processor, RT-safe method
                 * @return true if cancel request was sent to processor
                 */
                inline bool         cancelled() const { return bCancelled; }

                /**
                 * This method allows to cancel the execution of process() method by
                 * another thread, RT-safe method
                 */
                void                cancel() { if (!bCancelled) bCancelled = true; }

                /**
                 * Perform processing, non-RT-safe, should be launched in a thread
                 * @param threads number of threads used for processing
                 * @param initial initial energy of the signal
                 * @return status of operation
                 */
                status_t            process(size_t threads, float initial);
        };

    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_3D_RAYTRACE3D_H_ */
