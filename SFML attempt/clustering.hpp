#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <random>

class ClusteringMethod {
public:
	virtual void setData(const std::vector<std::vector<float>>& data) = 0;
	virtual void run() = 0;
	virtual int getClusterId(int i) = 0;
	virtual std::vector<int> getClusterSizes() = 0;
	virtual std::vector<float> getBiomeProb(int i) = 0;
	virtual std::vector<float> getCentroid(int i) = 0;
	virtual ~ClusteringMethod() = default;
};


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

class KMeans : public ClusteringMethod{
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

	void setData(const std::vector<std::vector<float>>& data) override {
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
			int bestCluster = 0; // NEEDS to be -1 
			float minDistance = std::numeric_limits<float>::max();

			for (int j = 0; j < clusters.size(); ++j) {
				float dist = distance(point, clusters[j].getCentroid());

				if (dist < minDistance) {
					minDistance = dist;
					bestCluster = j;
				}
			}
			// How does bestCluster end up being -1? Percepitation is nan so dist does not work..
			
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

	void run() override {
		init();
		bool done = false;
		int iter = 0;
		while (!done || iter < iters) {
			done = assignPoints();
			updateCentroids();
			iter++;
		}
	}

	int getClusterId(int index) override {
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

	std::vector<int> getClusterSizes() override {
		return clusterSizes;
	}

	std::vector<float> getBiomeProb(int index) override {
		std::vector<float> biomeProb(k, 0);

		biomeProb[getClusterId(index)] = 1.f;
		return biomeProb;
	}

};

class GMM : public ClusteringMethod {
	// Gaussian Mixture Models

	// There exist some issues where all probabilities are 0, unsure why?
private:
	int k;
	int dimensions;
	int iters;
	std::vector<std::vector<float>> data;
	std::vector<int> clusters;
	std::vector<std::vector<float>> probabilities;

	std::vector<float> mean;
	std::vector<float> stdDev;

	std::vector<float> mean_clusters;
	std::vector<float> stdDev_clusters;

	bool ocean = true; // If ocean is true , we will not standardize the first dimension
public:
	GMM(int k, int dimensions, int iters, bool ocean = true) : k(k), dimensions(dimensions), iters(iters), ocean(ocean) {}

	void setData(const std::vector<std::vector<float>>& data) override {
		this->data = data;

		clusters.resize(data.size(), -1);
		probabilities.resize(data.size(), std::vector<float>(k, 0.f));

		mean_clusters.resize(k * dimensions, 0.f);
		stdDev_clusters.resize(k * dimensions, 0.f);
	}

	void Init()
	{ // Here we need a random iteration and a way to manually decide.
		Standardize();
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dataIndex(0, data.size() - 1);
		for (int i = 0; i < k; ++i) {
			int index = dataIndex(gen);
			for (int j = 0; j < dimensions; ++j) {
				mean_clusters[i * dimensions + j] = data[index][j];
				stdDev_clusters[i * dimensions + j] = 1.0;
			}
		}
	}

	void Standardize()
	{	// get mean and standard deviation for each dimension
		int start_iter = 0;
		if (ocean) {
			start_iter = 1;
		}
		mean.resize(dimensions, 0);
		stdDev.resize(dimensions, 0);

		if (data.empty()) {
			return;
		}

		for (const auto& point : data) {
			for (int i = start_iter; i < dimensions; ++i) {
				mean[i] += point[i];
			}
		}

		for (int i = start_iter; i < dimensions; ++i) {
			mean[i] /= data.size();
		}

		for (const auto& point : data) {
			for (int i = start_iter; i < dimensions; ++i) {
				stdDev[i] += (point[i] - mean[i]) * (point[i] - mean[i]);
			}
		}

		for (int i = start_iter; i < dimensions; ++i) {
			stdDev[i] = sqrt(stdDev[i] / data.size());
		}

		// standardize data
		for (auto& point : data) {
			for (int i = start_iter; i < dimensions; ++i) {
				point[i] = (point[i] - mean[i]) / (stdDev[i] + 1e-8);
			}
		}
	}

	void run() override {
		float tolerance = 1e-4;
		float changeInMeans = 1.f;
		Init();
		std::vector<float> sumProb(k, 0.f);
		std::vector<float> sumMean(k * dimensions, 0.f);
		std::vector<float> sumStdDev(k * dimensions, 0.f);
		for (int iter = 0; iter < iters; iter++)
		{
			if (changeInMeans < tolerance) break;
			// E-step
			for (int i = 0; i < data.size(); ++i) {
				float sum = 0;
				for (int j = 0; j < k; ++j) {
					float dist = 0;
					for (int l = 0; l < dimensions; ++l) {
						float diff = data[i][l] - mean_clusters[j * dimensions + l];
						dist += diff * diff / ((2 * stdDev_clusters[j * dimensions + l] * stdDev_clusters[j * dimensions + l]) + 1e-8);
					}
					probabilities[i][j] = exp(-dist);
					sum += probabilities[i][j];
				}
				for (int j = 0; j < k; ++j) {
					sum += 1e-8; // Small regularization term
					probabilities[i][j] /= sum;
				}
			}
			// M-step
			std::fill(sumProb.begin(), sumProb.end(), 0.f);
			std::fill(sumMean.begin(), sumMean.end(), 0.f);
			std::fill(sumStdDev.begin(), sumStdDev.end(), 0.f);
			for (int i = 0; i < data.size(); ++i) {
				for (int j = 0; j < k; ++j) {
					sumProb[j] += probabilities[i][j];
					for (int l = 0; l < dimensions; ++l) {
						sumMean[j * dimensions + l] += data[i][l] * probabilities[i][j];
					}
				}
			}
			for (int j = 0; j < k; ++j) {
				for (int l = 0; l < dimensions; ++l) {
					mean_clusters[j * dimensions + l] = sumMean[j * dimensions + l] / (sumProb[j] + 1e-8);
					changeInMeans += abs(mean_clusters[j * dimensions + l] - mean_clusters[j * dimensions + l]);
				}
			}
			for (int i = 0; i < data.size(); ++i) {
				for (int j = 0; j < k; ++j) {
					for (int l = 0; l < dimensions; ++l) {
						float diff = data[i][l] - mean_clusters[j * dimensions + l];
						sumStdDev[j * dimensions + l] += diff * diff * probabilities[i][j];
					}
				}
			}
			for (int j = 0; j < k; ++j) {
				for (int l = 0; l < dimensions; ++l) {
					stdDev_clusters[j * dimensions + l] = sqrt(sumStdDev[j * dimensions + l] / (sumProb[j] + 1e-8));
				}
			}
		}

		// Assign clusters
		for (int i = 0; i < data.size(); ++i) {
			clusters[i] = chooseIndexMax(probabilities[i]);
		}
	}

	int getClusterId(int index) override {
		return clusters[index];
	}

	std::vector<float> getBiomeProb(int index) override {
		return probabilities[index];
	}

	std::vector<float> getCentroid(int clusterId)
	{
		std::vector<float> centroid;
		for (int i = 0; i < dimensions; ++i) {
			// Unstandardize the data
			mean_clusters[clusterId * dimensions + i] *= (stdDev[i] + 1e-8) + ocean;
			mean_clusters[clusterId * dimensions + i] += mean[i];

			centroid.push_back(mean_clusters[clusterId * dimensions + i]);
		}
		return centroid;
	}

	std::vector<float> getStdDev(int clusterId)
	{
		std::vector<float> stdDev;
		for (int i = 0; i < dimensions; ++i) {
			stdDev.push_back(stdDev_clusters[clusterId * dimensions + i]);
		}
		return stdDev;
	}

	std::vector<int> getClusters() const {
		return clusters;
	}

	std::vector<int> getClusterSizes() override {
		std::vector<int> clusterSizes(k, 0);
		for (int i = 0; i < clusters.size(); ++i) {
			if (clusters[i] >= 0 && clusters[i] < k) {
				clusterSizes[clusters[i]]++;
			}
		}
		return clusterSizes;
	}

};
