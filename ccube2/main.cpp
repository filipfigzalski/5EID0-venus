#include <opencv2/core/hal/interface.h>

#include <cstdio>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/core/cvstd.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <string>
#include <vector>

#define CAM_NUM 0

/* #define DISPLAY */
#define DEBUG

using namespace std;
using namespace cv;

// Function to perform Difference of Gaussians
Mat diff_gauss(const Mat &img, int k1, double s1, int k2, double s2) {
  Mat b1, b2;
  GaussianBlur(img, b1, Size(k1, k1), s1);
  GaussianBlur(img, b2, Size(k2, k2), s2);
  return b1 - b2;
}

struct HSL {
  double h, s, l;

  HSL(double h, double s, double l) : h(h), s(s), l(l){};
};

// Convert rgb to hsl
HSL rgb2hsl(double r, double g, double b) {
  // Find max and min value
  double maxVal = max({r, g, b});
  double minVal = min({r, g, b});
  double delta = maxVal - minVal;

  // Calculate lightness
  double L = (maxVal + minVal) / 2.0;

  // Calculate hue and saturation
  double H, S;
  if (delta == 0) {
    // If delta = 0, then saturation is 0 and hue is undefined
    S = 0;
    H = 0;
  } else {
    // Calculate saturation
    S = (L <= 0.5) ? (delta / (maxVal + minVal))
                   : (delta / (2.0 - maxVal - minVal));

    // Calculate hue
    if (maxVal == r) {
      H = (g - b) / delta + (g < b ? 6 : 0);
    } else if (maxVal == g) {
      H = (b - r) / delta + 2;
    } else {
      H = (r - g) / delta + 4;
    }

    H /= 6.0;
  }

  // Convert hue to degrees
  H *= 360.0;

  return HSL(H, S, L);
}

// Function to classify given hsv color
string classify_color(const Scalar &bgr_mean) {
  HSL hsl =
      rgb2hsl(bgr_mean[2] / 255.0, bgr_mean[1] / 255.0, bgr_mean[0] / 255.0);

#ifdef DEBUG
  printf("H: %f, S: %f, L: %f\n", hsl.h, hsl.s, hsl.l);
#endif

  if (hsl.l <= 0.10) {
    return "black";
  } else if (hsl.l > 0.10 && hsl.l < 0.55) {
    if (hsl.h < 20 || hsl.h > 340) {
      return "red";
    } else if (hsl.h > 80 && hsl.h < 160) {
      return "green";
    } else if (hsl.h > 180 && hsl.h < 280) {
      return "blue";
    }
  } else {
    return "white";
  }

  return "unknown";
}

int main() {
  // Access the webcam
  VideoCapture cam(CAM_NUM);
  if (!cam.isOpened()) {
    cerr << "Error: Could not open the camera." << endl;
    return -1;
  }

  // Set the resolution
//   cam.set(CAP_PROP_FRAME_WIDTH, 720); 
//   cam.set(CAP_PROP_FRAME_HEIGHT, 480); 

  cam.set(CAP_PROP_FRAME_WIDTH, 640);
  cam.set(CAP_PROP_FRAME_HEIGHT, 480);

  // Capture the frame
  Mat img;
  cam >> img;
  if (img.empty()) {
    cerr << "Error: Captured frame is empty." << endl;
    return -1;
  }
  cam.release();

  // Increase saturation
  Mat hsv_img;
  cvtColor(img, hsv_img, COLOR_BGR2HSV);
  vector<Mat> hsv_channels;
  split(hsv_img, hsv_channels);
  hsv_channels[1] *= 1.5;
  cv::min(hsv_channels[1], 255, hsv_channels[1]);
  merge(hsv_channels, hsv_img);
  cvtColor(hsv_img, img, COLOR_HSV2BGR);

  // Convert to gray
  vector<Mat> gray_channels(3);
  split(img, gray_channels);
  Mat gray = (gray_channels[0], gray_channels[1], gray_channels[2]) / 3;
  gray.convertTo(gray, CV_8U);

  // Perform Difference of Gaussians
  Mat DoG_img = diff_gauss(gray, 7, 7, 17, 17);

  // Apply Otsu Threshold
  Mat th;
  threshold(DoG_img, th, 127, 255, THRESH_BINARY | THRESH_OTSU);

  // Finding contours
  vector<vector<Point>> contours;
  findContours(th, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

  // Create copy of original image
  Mat img_out = img.clone();

  // For each contour above certain are and extend, draw minimum bounding box
  for (const auto &c : contours) {
    double area = contourArea(c);

    Rect boundingRect = cv::boundingRect(c);
    double extent = area / (boundingRect.width + boundingRect.height);

    // check if it is cube
    // double cubeness = abs((boundingRect.width + boundingRect.height)/boundingRect.width - 0.5);
    // printf("Cubeness: %lf\n", cubeness);

    if (area > 1200 && extent > 0.05) {
      // Find best matching boundary rectangle
      RotatedRect rect = minAreaRect(c);

      // get vertices and center of rect
      Point2f vertices[4];
      rect.points(vertices);
      Point2f center = rect.center;

      // draw bounding box
      for (int i = 0; i < 4; i++) {
        line(img_out, vertices[i], vertices[(i + 1) % 4], Scalar(0, 255, 2), 4);
      }

      // scale rectangle down
      for (int i = 0; i < 4; i++) {
        vertices[i] = center + 0.5 * (vertices[i] - center);
      }

      // Create a mask with the same size as the original image
      Mat mask = Mat::zeros(img.size(), CV_8UC1);

      // Fill the mask with the rectangle
      Point rook_points[1][4];
      for (int j = 0; j < 4; j++) {
        rook_points[0][j] = vertices[j];
      }
      const Point *ppt[1] = {rook_points[0]};
      int npt[] = {4};
      fillPoly(mask, ppt, npt, 1, Scalar(255));

      // Calculate the mean color inside the mask
      Scalar mean_bgr = mean(img, mask);

      // Get the color class of calculated value
      string color_class = classify_color(mean_bgr);
      string size_class =
          (rect.size.width * rect.size.height > 25000) ? "big" : "small";
      string text = size_class + " + " + color_class;
      putText(img_out, text, Point(boundingRect.x, boundingRect.y),
              FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 2, LINE_AA);
      cout << text << endl;
    }
  }

#ifdef DISPLAY
  imshow("Resutl", img_out);
  waitKey(0);
#else
  imwrite("last.png", img_out);
  imwrite("last_raw.png", th);
#endif

  return (0);
}
