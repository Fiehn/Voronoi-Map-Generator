#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <exception>

// Create a k-means clustering class to assign biomes from arbitrary number of variables

class Cluster {
private:
	int id;
	std::vector<float> centroid;
	std::vector<std::vector<float>> points;

public:
	Cluster(int id, std::vector<float> centroid, int point_count) : id(id), centroid(centroid) { points.reserve(point_count); }

	void addPoint(std::vector<float> point) {
		points.push_back(point);
	}

	void clearPoints() {
		points.clear();
	}

	const std::vector<std::vector<float>>& getPoints() const {
		return points;
	}

	const std::vector<float>& getCentroid() const {
		return centroid;
	}

	void setCentroid(std::vector<float>& newCentroid) {
		centroid = newCentroid;
	}

	int getId() const {
		return id;
	}

	int getSize() const {
		return points.size();
	}
};

class KMeans {
private:
	int k;
	int dimensions;
	int iters;
	std::vector<Cluster> clusters;
	std::vector<std::vector<float>> data;

	std::vector<int> clusterIds;
	std::vector<int> clusterSizes;

	std::vector<float> mean;
	std::vector<float> stdDev;

public:
	KMeans(int k, int dimensions, int iters) : k(k), dimensions(dimensions), iters(iters) { clusterSizes.resize(k, 0); }

	void setData(const std::vector<std::vector<float>>& data) {
		this->data = data;
		clusterIds.resize(data.size(), -1);
		standardize();
	}

	void init() {
		std::vector<int> usedIndices;
		std::size_t dataSize = data.size();

		for (int i = 0; i < k; ++i) {
			int index = rand() % dataSize;
			do {
				index = rand() % dataSize;
			} while (std::find(usedIndices.begin(), usedIndices.end(), index) != usedIndices.end());

			usedIndices.push_back(index);
			clusters.emplace_back(Cluster(i, data[index], dataSize / (k / 2)));
		}
	}
	
	void standardize() {
		// get mean and standard deviation for each dimension
		mean.resize(dimensions, 0);
		stdDev.resize(dimensions, 0);

		for (const auto& point : data) {
			for (int i = 0; i < dimensions; ++i) {
				mean[i] += point[i];
			}
		}

		for (int i = 0; i < dimensions; ++i) {
			mean[i] /= data.size();
		}

		for (const auto& point : data) {
			for (int i = 0; i < dimensions; ++i) {
				stdDev[i] += (point[i] - mean[i]) * (point[i] - mean[i]);
			}
		}

		for (int i = 0; i < dimensions; ++i) {
			stdDev[i] = sqrt(stdDev[i] / data.size());
		}

		// standardize data
		for (auto& point : data) {
			for (int i = 0; i < dimensions; ++i) {
				point[i] = (point[i] - mean[i]) / stdDev[i];
			}
		}
	}

	float distance(const std::vector<float>& a, const std::vector<float>& b) {
		float distance = 0;
		
		for (int i = 0; i < dimensions; ++i) {
			float diff = a[i] - b[i];
			distance += diff * diff;
		}
		return distance; // sqrt(distance) if needed
	}

	bool assignPoints() {
		bool done = true;
		
		#pragma omp parallel for reduction(&&: done) num_threads(16) schedule(static)
		for (int i = 0; i < data.size(); ++i) {
			const std::vector<float>& point = data[i];
			int bestCluster = -1;
			float minDistance = std::numeric_limits<float>::max();

			for (int j = 0; j < clusters.size(); ++j) {
				float dist = distance(point, clusters[j].getCentroid());

				if (dist < minDistance) {
					minDistance = dist;
					bestCluster = j;
				}
			}
			
			if (clusterIds[i] != bestCluster) {
				done = false;
			}
			clusterIds[i] = bestCluster;

			#pragma omp critical
			{
				clusters[bestCluster].addPoint(point);
			}
		}
		return done;
	}

	void updateCentroids() {
		#pragma omp parallel for num_threads(16) schedule(static)
		for (int i = 0; i < clusters.size(); ++i) {
			std::vector<float> newCentroid(dimensions, 0);
			const std::vector<std::vector<float>>& points = clusters[i].getPoints();
			int numPoints = points.size();

			if (numPoints == 0) {
				continue;
			}


			for (const auto& point : points) {
				for (int k = 0; k < dimensions; ++k) {
					newCentroid[k] += point[k];
				}
			}

			for (int k = 0; k < dimensions; ++k) {
				newCentroid[k] /= numPoints;
			}

			clusterSizes[i] = clusters[i].getSize();

			clusters[i].setCentroid(newCentroid);
			clusters[i].clearPoints();
		}
	}

	void run() {
		init();
		bool done = false;
		int iter = 0;
		while (!done || iter < iters) {
			done = assignPoints();
			updateCentroids();
			iter++;
		}
	}

	int getClusterId(int index) const {
		return clusterIds[index];
	}

	std::vector<float> getCentroid(int clusterId)
	{
		return clusters[clusterId].getCentroid();
	}

	std::vector<float> getCentroidUnstandard(int clusterId)
	{
		std::vector<float> centroid = clusters[clusterId].getCentroid();
		for (int i = 0; i < dimensions; ++i) {
			centroid[i] = centroid[i] * stdDev[i] + mean[i];
		}
		return centroid;
	}

	std::vector<int> getClusterSizes() const {
		return clusterSizes;
	}

};