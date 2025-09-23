from pydantic import BaseSettings, Field


class Settings(BaseSettings):
    env: str = Field('development', env='ENV')
    api_key: str = Field(..., env='ML_API_KEY')
    influx_url: str = Field(..., env='INFLUX_URL')
    influx_token: str = Field(..., env='INFLUX_TOKEN')
    influx_org: str = Field(..., env='INFLUX_ORG')
    influx_bucket: str = Field('iot_raw', env='INFLUX_BUCKET_RAW')
    mongo_uri: str = Field(..., env='MONGO_URI')
    model_store_path: str = Field('/opt/farmstack/ml-engine/models', env='MODEL_STORE_PATH')
    report_output_path: str = Field('/opt/farmstack/ml-engine/reports', env='REPORT_OUTPUT_PATH')
    tz: str = Field('Africa/Kinshasa', env='TZ')

    class Config:
        env_file = '.env'
        case_sensitive = False


def get_settings() -> Settings:
    return Settings()
