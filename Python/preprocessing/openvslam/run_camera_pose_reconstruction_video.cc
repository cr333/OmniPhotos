#include "util/image_util.h"

#ifdef USE_PANGOLIN_VIEWER
#include "pangolin_viewer/viewer.h"
#elif USE_SOCKET_PUBLISHER
#include "socket_publisher/publisher.h"
#endif

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include "openvslam/system.h"
#include "openvslam/config.h"

#include <iostream>
#include <chrono>
#include <numeric>

#include <opencv2/opencv.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <spdlog/spdlog.h>
#include <popl.hpp>

#ifdef USE_STACK_TRACE_LOGGER
#include <glog/logging.h>
#endif

#ifdef USE_GOOGLE_PERFTOOLS
#include <gperftools/profiler.h>
#endif

//void show_video_info(const std::string & video_file_path)
void show_opencv_video_handle_info(const cv::VideoCapture& cap) {
    //cv::VideoCapture cap(video_file_path);
    //std::cout << "The video information of " << video_file_path << std::endl;
    std::cout << "fps:" << static_cast<int>(cap.get(CV_CAP_PROP_FPS)) << std::endl;
    std::cout << "frame number: " << cap.get(CV_CAP_PROP_FRAME_COUNT) << std::endl;
    std::cout << "frame height: " << cap.get(CV_CAP_PROP_FRAME_HEIGHT) << std::endl;
    std::cout << "frame width: " << cap.get(CV_CAP_PROP_FRAME_WIDTH) << std::endl;
    std::cout << "current frame reading index at:" << cap.get(CV_CAP_PROP_POS_FRAMES) << std::endl;
    std::cout << "Current position of the video file in milliseconds: " << cap.get(CV_CAP_PROP_POS_MSEC) << std::endl;
}

void mono_localization_video(const std::shared_ptr<openvslam::config>& cfg,
                             const std::string& vocab_file_path, const std::string& video_path, const std::string& mask_img_path,
                             const std::string& map_db_path, const bool mapping,
                             const unsigned int frame_skip, const bool no_sleep, const bool auto_term,
                             const std::string& trajectory_file_path,
							 const double downsample_scalar = 1.0) {
    spdlog::info("mono_localization called to reconstruct the camera pose information");

    // load the mask image
     cv::Mat mask = cv::Mat{};
    if(!mask_img_path.empty() && 
        fs::exists(fs::path(mask_img_path)) && 
        !fs::is_directory(fs::path(mask_img_path)))
    {
        spdlog::info("use uniform mask for each frame, and load mask form " + mask_img_path);
        mask = cv::imread(mask_img_path, cv::IMREAD_GRAYSCALE);
    }else if(!mask_img_path.empty() && 
            fs::exists(fs::path(mask_img_path)) && 
            fs::is_directory(fs::path(mask_img_path))){
        spdlog::info("use different mask for each frame, and load mask form " + mask_img_path);
    }

    cv::VideoCapture video = cv::VideoCapture(video_path, cv::CAP_FFMPEG);
    unsigned int num_frame = video.get(CV_CAP_PROP_FRAME_COUNT);
    const int fps = static_cast<int>(video.get(CV_CAP_PROP_FPS) + 0.5);

    if (fps != static_cast<int>(cfg->camera_->fps_)) {
        std::string msg = "fps set error, the video FPS is ";
        msg = msg + std::to_string(fps);
        throw std::invalid_argument(msg);
    }

    // build a SLAM system
    openvslam::system SLAM(cfg, vocab_file_path);
    // load the prebuilt map
    SLAM.load_map_database(map_db_path);
    // startup the SLAM process (it does not need initialization of a map)
    SLAM.startup(false);
    // select to activate the mapping module or not
    if (mapping) {
        SLAM.enable_mapping_module();
    }
    else {
        SLAM.disable_mapping_module();
    }

    // create a viewer object
    // and pass the frame_publisher and the map_publisher
#ifdef USE_PANGOLIN_VIEWER
    pangolin_viewer::viewer viewer(cfg, &SLAM, SLAM.get_frame_publisher(), SLAM.get_map_publisher());
#elif USE_SOCKET_PUBLISHER
    socket_publisher::publisher publisher(cfg, &SLAM, SLAM.get_frame_publisher(), SLAM.get_map_publisher());
#endif

    std::vector<double> track_times;
    track_times.reserve(num_frame);

    double timestamp = 0.0;

    std::string msg("run localization step with video: ");
    msg = msg + video_path;
    spdlog::info(msg);
    show_opencv_video_handle_info(video);

    // run the SLAM in another thread
    int log_interval = 60;
    std::thread thread([&]() {
        for (unsigned int i = 0; i < num_frame; ++i) {
            const auto tp_0 = std::chrono::steady_clock::now();
            // get current frame data
            cv::Mat img;
            video.set(cv::CAP_PROP_POS_FRAMES, i);
            video >> img;

            if (downsample_scalar != 1.0) {
                cv::resize(img, img, cv::Size(), downsample_scalar, downsample_scalar);
            }

            const auto tp_1 = std::chrono::steady_clock::now();

            if(fs::is_directory(fs::path(mask_img_path)))
            {
                char index_str[100];
                snprintf(index_str, sizeof(index_str), "/%04d.jpg", i);
                std::string mask_path = mask_img_path + index_str;
                mask = cv::imread(mask_path, cv::IMREAD_GRAYSCALE);
                if (i % log_interval == 0) {
                    spdlog::debug("load mask file :" + mask_path);
                }
            }

            if (!img.empty() && (i % frame_skip == 0)) {
                // input the current frame and estimate the camera pose
                SLAM.feed_monocular_frame(img, timestamp, mask);
            }

            const auto tp_2 = std::chrono::steady_clock::now();

            const auto track_time = std::chrono::duration_cast<std::chrono::duration<double>>(tp_2 - tp_1).count();
            if (i % frame_skip == 0) {
                track_times.push_back(track_time);
            }

            // wait until the timestamp of the next frame
            if (!no_sleep && i < num_frame - 1) {
                const auto wait_time = 1.0 / cfg->camera_->fps_ - track_time;
                if (0.0 < wait_time) {
                    std::this_thread::sleep_for(std::chrono::microseconds(static_cast<unsigned int>(wait_time * 1e6)));
                }
            }

            if(i % log_interval == 0)
            {
                    const auto tp_3 = std::chrono::steady_clock::now();
                    const auto track_time_all = std::chrono::duration_cast<std::chrono::duration<double>>(tp_3 - tp_0).count();
                    const auto extract_frame_time = std::chrono::duration_cast<std::chrono::duration<double>>(tp_1 - tp_0).count();
                    std::stringstream ss;
                    ss << "frame " << i  << "/" << num_frame << " use time (s): " << track_time_all << " extracting frame use time (s): " << extract_frame_time << std::endl;
                    spdlog::debug(ss.str());
            }

            // check if the termination of SLAM system is requested or not
            if (SLAM.terminate_is_requested()) {
                break;
            }

            timestamp += 1.0 / cfg->camera_->fps_;
        }

        // wait until the loop BA is finished
        while (SLAM.loop_BA_is_running()) {
            std::this_thread::sleep_for(std::chrono::microseconds(5000));
        }

        // automatically close the viewer
#ifdef USE_PANGOLIN_VIEWER
        if (auto_term) {
            viewer.request_terminate();
        }
#elif USE_SOCKET_PUBLISHER
        if (auto_term) {
            publisher.request_terminate();
        }
#endif
    });

    // run the viewer in the current thread
#ifdef USE_PANGOLIN_VIEWER
    viewer.run();
#elif USE_SOCKET_PUBLISHER
    publisher.run();
#endif

    thread.join();

    // shutdown the SLAM process
    SLAM.shutdown();

    // output the trajectories for evaluation
    SLAM.save_frame_trajectory(trajectory_file_path, "TUM");

    std::sort(track_times.begin(), track_times.end());
    const auto total_track_time = std::accumulate(track_times.begin(), track_times.end(), 0.0);
    std::cout << "median tracking time: " << track_times.at(track_times.size() / 2) << "[s]" << std::endl;
    std::cout << "mean tracking time: " << total_track_time / track_times.size() << "[s]" << std::endl;
}

void mono_slam_video(const std::shared_ptr<openvslam::config>& cfg,
                     const std::string& vocab_file_path, const std::string& video_path, const std::string& mask_img_path,
                     const unsigned int frame_skip, const bool no_sleep, const bool auto_term,
                     const bool eval_log, const std::string& map_db_path,
                     const int repeat_times,
					 const double downsample_scalar = 1.0) {
    spdlog::info("mono_tracking called to reconstruct the map information");

    // load the mask image
     cv::Mat mask = cv::Mat{};
    if(!mask_img_path.empty() && 
        fs::exists(fs::path(mask_img_path)) && 
        !fs::is_directory(fs::path(mask_img_path)))
    {
        spdlog::info("use uniform mask for each frame, and load mask form " + mask_img_path);
        mask = cv::imread(mask_img_path, cv::IMREAD_GRAYSCALE);
    }else if(!mask_img_path.empty() && 
            fs::exists(fs::path(mask_img_path)) && 
            fs::is_directory(fs::path(mask_img_path)))
    {
        spdlog::info("use different mask for each frame, and load mask form " + mask_img_path);
    }

    // open video and load video information
    double timestamp = 0.0;
    cv::VideoCapture video = cv::VideoCapture(video_path, cv::CAP_FFMPEG);

    bool video_reverse_sequence = false; // the frame playback direction
    unsigned int num_frame = video.get(CV_CAP_PROP_FRAME_COUNT);
    const int fps = static_cast<int>(video.get(CV_CAP_PROP_FPS) + 0.5);

    if (fps != static_cast<int>(cfg->camera_->fps_)) {
        std::string msg = "fps set error, the video FPS is ";
        msg = msg + std::to_string(fps);
        throw std::invalid_argument(msg);
    }

    // build a SLAM system
    openvslam::system SLAM(cfg, vocab_file_path);
    // startup the SLAM process
    SLAM.startup();

#ifdef USE_PANGOLIN_VIEWER
    pangolin_viewer::viewer viewer(cfg, &SLAM, SLAM.get_frame_publisher(), SLAM.get_map_publisher());
#elif USE_SOCKET_PUBLISHER
    socket_publisher::publisher publisher(cfg, &SLAM, SLAM.get_frame_publisher(), SLAM.get_map_publisher());
#endif

    std::vector<double> track_times;
    track_times.resize(num_frame * repeat_times);

    std::string msg("run slam step with video : ");
    msg = msg + video_path;
    spdlog::info(msg);
    show_opencv_video_handle_info(video);

    // run the SLAM in another thread
    int log_interval = 60;
    std::thread thread([&]() {
        for (int loop_counter = 0; loop_counter < repeat_times; loop_counter++) {
            std::stringstream ss;
            ss << "run slam step in repeat time :" << loop_counter + 1 << "/" << repeat_times;

            if (loop_counter % 2 == 0) {
                video_reverse_sequence = false;
                ss << " forward video sequence.";
            }
            else {
                video_reverse_sequence = true;
                ss << " reverse video sequence.";
            }

            spdlog::info(ss.str());

            for (unsigned int i = 0; i < num_frame; ++i) {
                int current_frame_idx = -1;
                if (video_reverse_sequence) {
                    current_frame_idx = num_frame - i - 1;
                }
                else {
                    current_frame_idx = i;
                }

                const auto tp_0 = std::chrono::steady_clock::now();

                // get current frame data
                cv::Mat frame;
                video.set(cv::CAP_PROP_POS_FRAMES, current_frame_idx);
                video >> frame; // !! spend 0.5 second in 3840x1920 video 

                if (downsample_scalar != 1.0) {
                    cv::resize(frame, frame, cv::Size(), downsample_scalar , downsample_scalar);
                }

                const auto tp_1 = std::chrono::steady_clock::now();

                if(fs::is_directory(fs::path(mask_img_path)))
                {
                    char index_str[100];
                    snprintf(index_str, sizeof(index_str), "/%04d.jpg", current_frame_idx);
                    std::string mask_path = mask_img_path + index_str;
                    mask = cv::imread(mask_path, cv::IMREAD_GRAYSCALE);
                    if (i % log_interval == 0) {
                        spdlog::debug("load mask file :" + mask_path);
                    }
                }

                if (!frame.empty() && (current_frame_idx % frame_skip == 0)) {
                    // input the current frame and estimate the camera pose
                    SLAM.feed_monocular_frame(frame, timestamp, mask);
                }

                const auto tp_2 = std::chrono::steady_clock::now();

                const auto track_time = std::chrono::duration_cast<std::chrono::duration<double>>(tp_2 - tp_1).count();
                if (current_frame_idx % frame_skip == 0) {
                    track_times.push_back(track_time);
                }

                // wait until the timestamp of the next frame
                if (!no_sleep) {
                    const auto wait_time = 1.0 / cfg->camera_->fps_ - track_time;
                    if (0.0 < wait_time) {
                        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<unsigned int>(wait_time * 1e6)));
                    }
                }

                if(i % log_interval == 0)
                {
                    const auto tp_3 = std::chrono::steady_clock::now();
                    const auto track_time_all = std::chrono::duration_cast<std::chrono::duration<double>>(tp_3 - tp_0).count();
                    const auto extract_frame_time = std::chrono::duration_cast<std::chrono::duration<double>>(tp_1 - tp_0).count();
                    std::stringstream ss;
                    ss << "frame " << i  << "/" << num_frame << " use time (s): " << track_time_all << " extracting frame use time (s): " << extract_frame_time << std::endl;
                    spdlog::debug(ss.str());
                }

                timestamp += 1.0 / cfg->camera_->fps_;

                // check if the termination of SLAM system is requested or not
                if (SLAM.terminate_is_requested()) {
                    break;
                }
            }
        }

        // wait until the loop BA is finished
        while (SLAM.loop_BA_is_running()) {
            std::this_thread::sleep_for(std::chrono::microseconds(5000));
        }

        // automatically close the viewer
#ifdef USE_PANGOLIN_VIEWER
        if (auto_term) {
            viewer.request_terminate();
        }
#elif USE_SOCKET_PUBLISHER
        if (auto_term) {
            publisher.request_terminate();
        }
#endif
    });

    // run the viewer in the current thread
#ifdef USE_PANGOLIN_VIEWER
    viewer.run();
#elif USE_SOCKET_PUBLISHER
    publisher.run();
#endif

    thread.join();

    // shutdown the SLAM process
    SLAM.shutdown();

    if (!map_db_path.empty()) {
        // output the map database
        SLAM.save_map_database(map_db_path);
    }

    std::sort(track_times.begin(), track_times.end());
    const auto total_track_time = std::accumulate(track_times.begin(), track_times.end(), 0.0);
    std::cout << "median tracking time: " << track_times.at(track_times.size() / 2) << "[s]" << std::endl;
    std::cout << "mean tracking time: " << total_track_time / track_times.size() << "[s]" << std::endl;
}

int main(int argc, char* argv[]) {
#ifdef USE_STACK_TRACE_LOGGER
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();
#endif

    // create options
    popl::OptionParser op("Allowed options");
    auto help = op.add<popl::Switch>("h", "help", "produce help message");
    auto vocab_file_path = op.add<popl::Value<std::string>>("v", "vocab", "vocabulary file path");
    //auto img_dir_path = op.add<popl::Value<std::string>>("i", "img-dir", "directory path which contains images");
    auto video_path = op.add<popl::Value<std::string>>("i", "video-path", "the input video's path");
    auto config_file_path = op.add<popl::Value<std::string>>("c", "config", "config file path");
    auto mask_img_path = op.add<popl::Value<std::string>>("", "mask", "mask image path", "");
    auto frame_skip = op.add<popl::Value<unsigned int>>("", "frame-skip", "interval of frame skip", 1);
    auto no_sleep = op.add<popl::Switch>("", "no-sleep", "not wait for next frame in real time");
    auto auto_term = op.add<popl::Switch>("", "auto-term", "automatically terminate the viewer");
    auto debug_mode = op.add<popl::Switch>("", "debug", "debug mode");
    auto map_db_path = op.add<popl::Value<std::string>>("p", "map-db", "store a map database at this path after SLAM", "");
    auto recon_type = op.add<popl::Value<std::string>>("t", "recon-type", "specify the type of info to be reconstructed, optional [slam|localization|full]", "full");
	auto image_downsample_scalar = op.add<popl::Value<double>>("", "downsample_scalar", "the ratio of image downsample", 1.0);

    // parameters of mono_tracking
    auto eval_log = op.add<popl::Switch>("", "eval-log", "store trajectory and tracking times for evaluation");
    auto repeat_times = op.add<popl::Value<unsigned int>>("r", "repeat-times", "repeat time of reconstruction with the same video", 2);

    // parameters of mono_localization
    auto mapping = op.add<popl::Switch>("", "mapping", "perform mapping as well as localization");
    auto trajectory_path = op.add<popl::Value<std::string>>("", "trajectory_path", "the path of output frame_trajectory.txt");

    try {
        op.parse(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << std::endl;
        std::cerr << op << std::endl;
        return EXIT_FAILURE;
    }

    // check validness of options
    if (help->is_set()) {
        std::cerr << op << std::endl;
        return EXIT_FAILURE;
    }
    if (!vocab_file_path->is_set() || !video_path->is_set() || !config_file_path->is_set()) {
        std::cerr << "invalid arguments" << std::endl;
        std::cerr << std::endl;
        std::cerr << op << std::endl;
        return EXIT_FAILURE;
    }

    // setup logger
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%L] %v%$");
    if (debug_mode->is_set()) {
        spdlog::set_level(spdlog::level::debug);
    }
    else {
        spdlog::set_level(spdlog::level::info);
    }

    // load configuration
    std::shared_ptr<openvslam::config> cfg;
    try {
        cfg = std::make_shared<openvslam::config>(config_file_path->value());
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

#ifdef USE_GOOGLE_PERFTOOLS
    ProfilerStart("slam.prof");
#endif

    // run tracking or localization
    if (cfg->camera_->setup_type_ == openvslam::camera::setup_type_t::Monocular) {
        if (0 == recon_type->value().compare("slam") || 0 == recon_type->value().compare("full")) { // reconstruct scene map
            //check file status
            mono_slam_video(cfg, vocab_file_path->value(), video_path->value(), mask_img_path->value(),
                            frame_skip->value(), no_sleep->is_set(), auto_term->is_set(),
                            eval_log->is_set(), map_db_path->value(),
                            repeat_times->value(), image_downsample_scalar->value());
        }
        if (0 == recon_type->value().compare("localization") || 0 == recon_type->value().compare("full")) { // reconstruct camera pose
                                                                                                            //check file status
            mono_localization_video(cfg, vocab_file_path->value(), video_path->value(), mask_img_path->value(),
                                    map_db_path->value(), mapping->is_set(),
                                    frame_skip->value(), no_sleep->is_set(), auto_term->is_set(),
                                    trajectory_path->value(), image_downsample_scalar->value());
        }
    }
    else {
        throw std::runtime_error("Invalid setup type: " + cfg->camera_->get_setup_type_string());
    }

#ifdef USE_GOOGLE_PERFTOOLS
    ProfilerStop();
#endif

    return EXIT_SUCCESS;
}
