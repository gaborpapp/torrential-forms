#pragma once

namespace tf {

class GlobalSettings
{
    private:
        //! Singleton implementation
        GlobalSettings() {}
        ~GlobalSettings() {};

    public:
        static GlobalSettings& get() { static GlobalSettings data; return data; }

		float mEmitterRadiusMin;
		float mEmitterRadiusMax;
		float mEmitterRadiusStep;
		float mEmitterRadiusDamping;
		float mEmitterRepulsion;
		float mEmitterRepulsionRadius;
};

} // namespace tf
