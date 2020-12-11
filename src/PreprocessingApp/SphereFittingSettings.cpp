#include "SphereFittingSettings.hpp"


SphereFittingSettings::SphereFittingSettings() :
    SettingsSection("SphereFitting")
{
	addEntry("Enabled",                   enabled);
	addEntry("PolarSteps",                polar_steps);
	addEntry("AzimuthSteps",              azimuth_steps);
	addVectorEntry("DataWeight",          data_weight);
	addVectorEntry("RobustDataLoss",      robust_data_loss);
	addVectorEntry("RobustDataLossScale", robust_data_loss_scale);
	addVectorEntry("SmoothnessWeight",    smoothness_weight);
	addVectorEntry("PriorWeight",         prior_weight);
}
