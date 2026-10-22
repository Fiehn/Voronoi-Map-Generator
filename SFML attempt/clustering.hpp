#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <random>
#include <algorithm>
#include <numeric>
#include <limits>
#include <cmath>

class ClusteringMethod {
public:
	virtual void setData(const std::vector<std::vector<float>>& data) = 0;
	virtual void run() = 0;
	virtual int getClusterId(int i) = 0;
	virtual std::vector<int> getClusterSizes() = 0;
	virtual std::vector<float> getBiomeProb(int i) = 0;
	virtual std::vector<float> getCentroid(int i) = 0; // In the units of the data given to setData
	virtual ~ClusteringMethod() = default;
};

// k-means++: k different indices into the data to start the clusters on. The first is random, every next one is picked
// with a chance proportional to its squared distance to the closest start so far, which spreads the starts over the data
// k must not exceed the amount of points
inline std::vector<int> kmeansPlusPlusStart(const std::vector<std::vector<float>>& data, int k, std::mt19937& gen) {
	const int n = static_cast<int>(data.size());
	std::vector<int> start;
	std::vector<bool> used(n, false);
	std::vector<double> closest(n, std::numeric_limits<double>::max());

	start.push_back(std::uniform_int_distribution<int>(0, n - 1)(gen));
	used[start[0]] = true;

	while (static_cast<int>(start.size()) < k) {
		const std::vector<float>& last = data[start.back()];
		double total = 0.0;
		for (int i = 0; i < n; ++i) {
			double dist = 0.0;
			for (std::size_t d = 0; d < last.size(); ++d) {
				double diff = data[i][d] - last[d];
				dist += diff * diff;
			}
			closest[i] = std::min(closest[i], dist);
			if (!used[i]) total += closest[i];
		}

		int next = -1;
		if (total > 0.0) {
			double pick = std::uniform_real_distribution<double>(0.0, total)(gen);
			for (int i = 0; i < n; ++i) {
				if (used[i] || closest[i] <= 0.0) continue;
				next = i; // Kept as the fallback when rounding leaves a little of pick over
				pick -= closest[i];
				if (pick <= 0.0) break;
			}
		}
		else {
			// Every point left sits on a start already, any unused one will do
			int pick = std::uniform_int_distribution<int>(0, n - static_cast<int>(start.size()) - 1)(gen);
			for (int i = 0; i < n; ++i) {
				if (used[i]) continue;
				if (pick-- == 0) { next = i; break; }
			}
		}
		used[next] = true;
		start.push_back(next);
	}
	return start;
}


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

class KMeans : public ClusteringMethod {
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
		clusters.clear();
		std::mt19937 gen(std::random_device{}());
		std::vector<int> start = kmeansPlusPlusStart(data, k, gen);

		for (int i = 0; i < k; ++i) {
			clusters.emplace_back(Cluster(i, data[start[i]], static_cast<int>(data.size() / k) + 1));
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
				point[i] = (point[i] - mean[i]) / (stdDev[i] + 1e-8f);
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

			clusterSizes[i] = numPoints;
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

			clusters[i].setCentroid(newCentroid);
			clusters[i].clearPoints();
		}
	}

	void run() override {
		init();
		bool done = false;
		int iter = 0;
		while (!done && iter < iters) {
			done = assignPoints();
			updateCentroids();
			iter++;
		}
	}

	int getClusterId(int index) override {
		return clusterIds[index];
	}

	std::vector<float> getCentroid(int clusterId) override
	{
		std::vector<float> centroid = clusters[clusterId].getCentroid();
		for (int i = 0; i < dimensions; ++i) {
			centroid[i] = centroid[i] * (stdDev[i] + 1e-8f) + mean[i];
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

// Fills lower (d x d, row major) with L where L * L^T = matrix and returns log(det(matrix))
// The matrix must be symmetric positive definite, the GMM makes sure of that with the floor on the diagonal
inline double choleskyLogDet(const double* matrix, double* lower, int d) {
	double logDet = 0.0;
	for (int r = 0; r < d; ++r) {
		for (int c = 0; c < d; ++c) {
			if (c > r) {
				lower[r * d + c] = 0.0;
				continue;
			}
			double sum = matrix[r * d + c];
			for (int m = 0; m < c; ++m) {
				sum -= lower[r * d + m] * lower[c * d + m];
			}
			if (r == c) {
				lower[r * d + r] = std::sqrt(std::max(sum, 1e-12));
				logDet += 2.0 * std::log(lower[r * d + r]);
			}
			else {
				lower[r * d + c] = sum / lower[c * d + c];
			}
		}
	}
	return logDet;
}

class GMM : public ClusteringMethod {
	// Gaussian Mixture Models, full covariances and equal weights for every cluster
	// The full covariance lets correlated features (humidity is mostly made from temperature and precipitation) count once instead of twice
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
	std::vector<double> covariance_clusters; // k matrices of dimensions x dimensions, row major
	std::vector<double> cholesky_clusters; // Lower triangular factor of each covariance
	std::vector<double> logDet_clusters;

	// In standardised units, added to the diagonal of every covariance as a variance. Stops a cluster shrinking onto identical values
	// (distance to ocean is whole cells) or onto an exact relationship between features (ocean humidity is a line in temperature),
	// where -log(det) would grow without limit
	static constexpr float MIN_STDDEV = 0.05f;

	// EM only finds a local best, so it runs this many times from different starting points and keeps the most likely result
	// Every restart costs a full run
	static constexpr int RESTARTS = 3;

public:
	GMM(int k, int dimensions, int iters) : k(k), dimensions(dimensions), iters(iters) {}

	void setData(const std::vector<std::vector<float>>& data) override {
		this->data = data;
		Standardize();

		clusters.resize(data.size(), -1);
		probabilities.resize(data.size(), std::vector<float>(k, 0.f));

		mean_clusters.resize(k * dimensions, 0.f);
		covariance_clusters.resize(k * dimensions * dimensions, 0.0);
		cholesky_clusters.resize(k * dimensions * dimensions, 0.0);
		logDet_clusters.resize(k, 0.0);
	}

	void Init(std::mt19937& gen)
	{
		std::vector<int> start = kmeansPlusPlusStart(data, k, gen);
		const int d2 = dimensions * dimensions;
		for (int i = 0; i < k; ++i) {
			for (int j = 0; j < dimensions; ++j) {
				mean_clusters[i * dimensions + j] = data[start[i]][j];
			}
			// Every cluster starts as the identity, the spread of the standardised data
			for (int j = 0; j < d2; ++j) {
				covariance_clusters[i * d2 + j] = (j % (dimensions + 1) == 0) ? 1.0 : 0.0;
			}
			logDet_clusters[i] = choleskyLogDet(&covariance_clusters[i * d2], &cholesky_clusters[i * d2], dimensions);
		}
	}

	void Standardize()
	{	// get mean and standard deviation for each dimension
		mean.resize(dimensions, 0);
		stdDev.resize(dimensions, 0);

		if (data.empty()) {
			return;
		}

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
				point[i] = (point[i] - mean[i]) / (stdDev[i] + 1e-8f);
			}
		}
	}

	void run() override {
		std::mt19937 gen(std::random_device{}());
		double bestLikelihood = -std::numeric_limits<double>::infinity();
		std::vector<float> bestMeans;
		std::vector<double> bestCovariance, bestCholesky, bestLogDet;
		std::vector<std::vector<float>> bestProbabilities;

		for (int restart = 0; restart < std::max(RESTARTS, 1); ++restart) {
			double likelihood = runOnce(gen);
			if (likelihood > bestLikelihood || bestMeans.empty()) {
				bestLikelihood = likelihood;
				bestMeans = mean_clusters;
				bestCovariance = covariance_clusters;
				bestCholesky = cholesky_clusters;
				bestLogDet = logDet_clusters;
				bestProbabilities = probabilities;
			}
		}
		mean_clusters = std::move(bestMeans);
		covariance_clusters = std::move(bestCovariance);
		cholesky_clusters = std::move(bestCholesky);
		logDet_clusters = std::move(bestLogDet);
		probabilities = std::move(bestProbabilities);

		// Assign clusters
		for (int i = 0; i < data.size(); ++i) {
			clusters[i] = static_cast<int>(std::max_element(probabilities[i].begin(), probabilities[i].end()) - probabilities[i].begin());
		}
	}

	// One EM run from k-means++ starting points. Returns the log-likelihood of the last E-step, without the constant
	double runOnce(std::mt19937& gen) {
		float tolerance = 1e-4f;
		Init(gen);
		double likelihood = 0.0;
		std::vector<float> sumProb(k, 0.f);
		std::vector<float> sumMean(k * dimensions, 0.f);
		const int d2 = dimensions * dimensions;
		std::vector<double> sumCovariance(k * d2, 0.0);
		std::vector<float> previousMeans(k * dimensions, 0.f);
		std::vector<double> whitened(dimensions, 0.0);
		for (int iter = 0; iter < iters; iter++)
		{
			// E-step
			likelihood = 0.0;
			for (int i = 0; i < data.size(); ++i) {
				float minDist = std::numeric_limits<float>::max();
				for (int j = 0; j < k; ++j) {
					// Negative log of the Gaussian density, without the constant: half the squared Mahalanobis distance plus half the log determinant
					// The determinant makes wide clusters pay for their width, without it the widest cluster wins every point that is not close to another centre
					// Solving L * whitened = diff gives the Mahalanobis distance as the squared length of whitened
					const double* lower = &cholesky_clusters[j * d2];
					double mahalanobis = 0.0;
					for (int r = 0; r < dimensions; ++r) {
						double sum = data[i][r] - mean_clusters[j * dimensions + r];
						for (int m = 0; m < r; ++m) {
							sum -= lower[r * dimensions + m] * whitened[m];
						}
						whitened[r] = sum / lower[r * dimensions + r];
						mahalanobis += whitened[r] * whitened[r];
					}
					float dist = static_cast<float>(0.5 * mahalanobis + 0.5 * logDet_clusters[j]);
					probabilities[i][j] = dist;
					minDist = std::min(minDist, dist);
				}
				// Measured from the closest cluster so exp does not underflow to 0 for every cluster, the sum is always at least 1
				float sum = 0;
				for (int j = 0; j < k; ++j) {
					probabilities[i][j] = exp(-(probabilities[i][j] - minDist));
					sum += probabilities[i][j];
				}
				for (int j = 0; j < k; ++j) {
					probabilities[i][j] /= sum;
				}
				// log of the summed densities, with the closest cluster's taken back out
				likelihood += -minDist + std::log(sum);
			}
			// M-step
			std::fill(sumProb.begin(), sumProb.end(), 0.f);
			std::fill(sumMean.begin(), sumMean.end(), 0.f);
			std::fill(sumCovariance.begin(), sumCovariance.end(), 0.0);
			for (int i = 0; i < data.size(); ++i) {
				for (int j = 0; j < k; ++j) {
					sumProb[j] += probabilities[i][j];
					for (int l = 0; l < dimensions; ++l) {
						sumMean[j * dimensions + l] += data[i][l] * probabilities[i][j];
					}
				}
			}
			previousMeans = mean_clusters;
			float changeInMeans = 0.f;
			for (int j = 0; j < k; ++j) {
				for (int l = 0; l < dimensions; ++l) {
					mean_clusters[j * dimensions + l] = sumMean[j * dimensions + l] / (sumProb[j] + 1e-8);
					changeInMeans += std::abs(mean_clusters[j * dimensions + l] - previousMeans[j * dimensions + l]);
				}
			}
			// Only the lower triangle is summed, the covariance is symmetric
			for (int i = 0; i < data.size(); ++i) {
				for (int j = 0; j < k; ++j) {
					double p = probabilities[i][j];
					if (p < 1e-12) continue;
					double* sum = &sumCovariance[j * d2];
					const float* centre = &mean_clusters[j * dimensions];
					for (int r = 0; r < dimensions; ++r) {
						double diffR = data[i][r] - centre[r];
						for (int c = 0; c <= r; ++c) {
							sum[r * dimensions + c] += p * diffR * (data[i][c] - centre[c]);
						}
					}
				}
			}
			const double varianceFloor = static_cast<double>(MIN_STDDEV) * MIN_STDDEV;
			for (int j = 0; j < k; ++j) {
				double* covariance = &covariance_clusters[j * d2];
				for (int r = 0; r < dimensions; ++r) {
					for (int c = 0; c <= r; ++c) {
						double value = sumCovariance[j * d2 + r * dimensions + c] / (sumProb[j] + 1e-8);
						if (r == c) value += varianceFloor;
						covariance[r * dimensions + c] = value;
						covariance[c * dimensions + r] = value;
					}
				}
				logDet_clusters[j] = choleskyLogDet(covariance, &cholesky_clusters[j * d2], dimensions);
			}
			if (changeInMeans < tolerance) break;
		}
		return likelihood;
	}

	int getClusterId(int index) override {
		return clusters[index];
	}

	std::vector<float> getBiomeProb(int index) override {
		return probabilities[index];
	}

	std::vector<float> getCentroid(int clusterId) override
	{
		std::vector<float> centroid;
		for (int i = 0; i < dimensions; ++i) {
			// Unstandardize the data
			centroid.push_back(mean_clusters[clusterId * dimensions + i] * (stdDev[i] + 1e-8f) + mean[i]);
		}
		return centroid;
	}

	std::vector<float> getStdDev(int clusterId)
	{
		std::vector<float> stdDev;
		for (int i = 0; i < dimensions; ++i) {
			// Standardised units, the diagonal of the covariance
			stdDev.push_back(static_cast<float>(std::sqrt(covariance_clusters[clusterId * dimensions * dimensions + i * (dimensions + 1)])));
		}
		return stdDev;
	}

	// Standardised units, dimensions x dimensions row major
	std::vector<double> getCovariance(int clusterId) const {
		auto begin = covariance_clusters.begin() + clusterId * dimensions * dimensions;
		return std::vector<double>(begin, begin + dimensions * dimensions);
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
