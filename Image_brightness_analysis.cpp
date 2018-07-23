#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include <algorithm>
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

int main() {
    cout << fixed << setprecision(3);
    string type, path;
    char sup;
    cout << "Enter the type of resource: 'video' or 'image'" << endl;
    cin >> type;

    if (type == "image") {
        //interaction with user
        cout << "Enter path to image: " << endl;
        cin >> path;
        cout << "Do you want to get supporting data (histogram, etc.)? Enter '0' for 'no' and '1' for 'yes'" << endl;
        cin >> sup;

        //new object to store image
        Mat image = imread(path);

        //if 'path' is wrong
        if (image.empty()) {
            cout << "Cannot open or find image file!" << endl;
            cin.get();
            return -1;
        }

        //visualising the image
        string windowName = "Image visualization window";
        namedWindow(windowName, WINDOW_NORMAL);
        imshow(windowName, image);

        //convertation to HSV color space
        cvtColor(image, image, CV_RGB2HSV);

        //split image in channels: H, S, V
        vector<Mat> hsv_planes;
        split(image, hsv_planes);
        long long pix_numb = image.rows*image.cols;

        //configuring histogram
        int histSize = 256; //V - from 0 to 255

        float range[] = { 0, histSize };
        const float* histRange = { range };
        bool uniform = true,
            accumulate = false;

        Mat hist; //resulting histogram
        calcHist(&hsv_planes[2], 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

        //calculating mathematical expectation
        //discrete random variable, M(x) = sum(x * p(x))
        //x - value of brightness (i)
        //p(x) - appropriate value from hisogram (hist[i]) divided by total number of pixels (pix_numb)

        double a = 0,
            sum = hist.at<float>(0);
        for (int i = 1; i < histSize; ++i) {
            a += i * hist.at<float>(i);
            sum += hist.at<float>(i);
        }
        a = a * 1. / pix_numb;

        //calculating dispersion
        //discrete random variable, D = M(x^2) - (M(x))^2
        //M(x^2) = sum(x^2 * p(x))

        double a_sqr = 0;
        for (int i = 1; i < histSize; ++i) {
            a_sqr += i * i * hist.at<float>(i);
        }
        a_sqr = a_sqr * 1. / pix_numb;
        double d = a_sqr - a * a; //dispersion
        d = sqrt(d);

        //supporting data for user
        if (sup == '1') {
            cout << "Mathematical expectation (a): " << a << endl;
            cout << "Standard error (d): " << d << endl;
        }

        //maximum value of all the histogram (and it's brightness value)
        double max = 0;
        int max_i = 0;
        for (int i = 0; i < histSize; ++i) {
            if (hist.at<float>(i) > max) {
                max = hist.at<float>(i);
                max_i = i;
            }
        }

        //maximum value between 55 and 200 (and it's brightness value)
        double max_55 = 0;
        int max_55_i = 55;
        for (int i = 55; i < 200; ++i) {
            if (hist.at<float>(i) > max_55) {
                max_55 = hist.at<float>(i);
                max_55_i = i;
            }
        }

        //trying to find out if the image has normal level of brightness
        double eps = 1e-3,
            left = a - d,
            right = a + d,
            middle = 128;
        if ((middle - left > eps) && (right - middle > eps)) {
            if (max_i >= 200) {
                if (max_55 * 1. / max < 0.7)
                    cout << "Image is too bright!" << endl;
                else {
                    if (max_i < 55)
                        cout << "Image is too dark!" << endl;
                    else
                        cout << "Image has normal level of brightness" << endl;
                }
            }
            else {
                if (max_i == max_55_i)
                    cout << "Image has normal level of brightness" << endl;
                else
                    cout << "Image is too dark!" << endl;
            }
        }
        else {
            if (middle - right > eps)
                cout << "Image is too dark" << endl;
            else
                cout << "Image is too bright" << endl;
        }

        //drawing histogram (optional)
        if (sup == '1') {
            int hist_w = 512,
                hist_h = 400;
            int bin_w = cvRound((double)hist_w / histSize);
            Mat histImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));

            normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

            for (int i = 1; i < histSize; ++i) {
                line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(hist.at<float>(i - 1))),
                    Point(bin_w*(i), hist_h - cvRound(hist.at<float>(i))),
                    Scalar(0, 0, 255), 1, 8, 0);
            }
            string name = "Brightness histogram";
            namedWindow(name, CV_WINDOW_AUTOSIZE);
            imshow(name, histImage);
        }

        //EXIT (press any key)
        waitKey(0);
        destroyAllWindows();
        //destroyWindow(windowName); //is not essential
    }
    else {
        if (type == "video") {
            //interaction with user
            cout << "Enter path to video: ";
            cin >> path;
            cout << "Do you want to get supporting data (histogram, etc.)? Enter '0' for 'no' and '1' for 'yes'" << endl;
            cin >> sup;

            VideoCapture cap(path);

            //if 'path' is wrong
            if (!cap.isOpened()) {
                cout << "Cannot open video file " << path << endl;
                cin.get();
                return -1;
            }

            //number of frames in video
            double frame_number = cap.get(CAP_PROP_FRAME_COUNT);
            double bad_frame = 0;

            //creating a window for the video
            String window_name = "Video visualization window";
            namedWindow(window_name, WINDOW_NORMAL);

            //video frame by frame
            while (true) {
                Mat frame;
                bool is_read = cap.read(frame);
                if (!is_read) {
                    cout << "End of video file!" << endl;
                    break;
                }
                //visualization of frame
                imshow(window_name, frame);

                //convertation to HSV color space
                cvtColor(frame, frame, CV_BGR2HSV);

                //split image in channels: H, S, V
                vector<Mat> hsv_planes;
                split(frame, hsv_planes);
                long long pix_numb = frame.rows*frame.cols;

                //configuring histogram
                int histSize = 256; //V - from 0 to 255
                float range[] = { 0, histSize };
                const float* histRange = { range };
                bool uniform = true,
                    accumulate = false;

                Mat hist; //resulting histogram
                calcHist(&hsv_planes[2], 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

                //calculating mathematical expectation
                //discrete random variable, 
                // M(x) = sum(x * p(x))
                //x - value of brightness (i)
                //p(x) - appropriate value from hisogram (hist[i]) divided by total number of pixels (pix_numb)
                double a = 0;
                double sum = hist.at<float>(0);
                for (int i = 1; i < histSize; ++i) {
                    a += i * hist.at<float>(i);
                    sum += hist.at<float>(i);
                }
                a = a * 1. / pix_numb;

                //calculating dispersion
                //discrete random variable, 
                // D = M(x^2) - (M(x))^2
                // M(x^2) = sum(x^2 * p(x))
                double a_sqr = 0;
                for (int i = 1; i < histSize; ++i) {
                    a_sqr += i * i * hist.at<float>(i);
                }
                a_sqr = a_sqr * 1. / pix_numb;
                double d = a_sqr - a * a; //dispersion
                d = sqrt(d);

                //maximum value of all the histogram (and it's brightness value)
                double max = 0;
                int max_i = 0;
                for (int i = 0; i < histSize; ++i) {
                    if (hist.at<float>(i) > max) {
                        max = hist.at<float>(i);
                        max_i = i;
                    }
                }

                //maximum value between 55 and 200 (and it's brightness value)
                double max_55 = 0;
                int max_55_i = 55;
                for (int i = 55; i < 200; ++i) {
                    if (hist.at<float>(i) > max_55) {
                        max_55 = hist.at<float>(i);
                        max_55_i = i;
                    }
                }

                //trying to find out if the frame has normal level of brightness

                double eps = 1e-3,
                    left = a - d,
                    right = a + d,
                    middle = 128;

                if ((middle - left > eps) && (right - middle > eps)) {
                    if (max_i >= 200) {
                        if (max_55 * 1. / max < 0.6)
                            ++bad_frame;
                        else {
                            if (max_i < 55)
                                ++bad_frame;
                        }
                    }
                    else {
                        if (max_i != max_55_i)
                            ++bad_frame;
                    }
                }
                else {
                    ++bad_frame;
                }

                //drawing histogram (optional)
                if (sup == '1') {
                    int hist_w = 512,
                        hist_h = 400;
                    int bin_w = cvRound((double)hist_w / histSize);
                    Mat histImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));

                    normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

                    for (int i = 1; i < histSize; ++i) {
                        line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(hist.at<float>(i - 1))),
                            Point(bin_w*(i), hist_h - cvRound(hist.at<float>(i))),
                            Scalar(0, 0, 255), 1, 8, 0);
                    }
                    string name = "Brightness histogram";
                    namedWindow(name, CV_WINDOW_AUTOSIZE);
                    imshow(name, histImage);
                }

                if (waitKey(10) == 27)
                {
                    cout << "Esc key is pressed by user. Stopping the video" << endl;
                    break;
                }
            }

            //percentage of bad frames
            double qual = bad_frame * 100.0 / frame_number;

            //supporting data for user
            if (sup == '1') {
                cout << "Total number of frames: " << frame_number << endl;
                cout << "Number of bad frames: " << bad_frame << endl;
                cout << "Percentage of bad frames in video: " << qual << endl;
            }

            // quality of video
            if (qual > 40.0) {
                cout << "Video is too lighted" << endl;
            }
            else {
                cout << "Video is normal" << endl;
            }
        }
        //if wrong type of input resource was entered
        else {
            cout << "Wrong type of input resource!" << endl;
        }
    }
    return 0;
}
