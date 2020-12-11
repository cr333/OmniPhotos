#pragma once

#include "Utils/ConfigReader.hpp"
#include "Utils/SettingsSection.hpp"


class SphereFittingSettings : public SettingsSection
{
public:
	SphereFittingSettings();

	// Enables sphere fitting.
	bool enabled = true;

	// Number of mesh subdivisions along the polar angle, i.e. between the poles.
	int polar_steps = 80;

	// Number of mesh subdivisions along the equator.
	int azimuth_steps = 160;

	// Weight of the data term.
	std::vector<double> data_weight { 1. };

	// Robust loss to use for the data term.
	std::vector<int> robust_data_loss { 1 /* RobustLoss::Huber */ };

	// Scale factor for the robust loss.
	std::vector<double> robust_data_loss_scale { 0.1 };

	// Weight of the smoothness term.
	std::vector<double> smoothness_weight = { 100. };

	// Weight of the prior term.
	std::vector<double> prior_weight { 0.001 };
};
