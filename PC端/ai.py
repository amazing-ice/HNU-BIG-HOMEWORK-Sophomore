from sparkai.llm.llm import ChatSparkLLM
from sparkai.core.messages import ChatMessage

# 配置信息
SPARKAI_URL = 'wss://spark-api.xf-yun.com/v3.5/chat'
SPARKAI_APP_ID = ''
SPARKAI_API_SECRET = ''
SPARKAI_API_KEY = ''
SPARKAI_DOMAIN = 'generalv3.5'


def get_llm_response(question: str) -> str:
    """
    调用星火大模型获取回答

    参数:
        question: 要问大模型的问题字符串

    返回:
        大模型返回的回答字符串
    """
    # 初始化大模型客户端
    spark = ChatSparkLLM(
        spark_api_url=SPARKAI_URL,
        spark_app_id=SPARKAI_APP_ID,
        spark_api_key=SPARKAI_API_KEY,
        spark_api_secret=SPARKAI_API_SECRET,
        spark_llm_domain=SPARKAI_DOMAIN,
        streaming=False,
    )

    # 构建消息
    messages = [ChatMessage(
        role="user",
        content=question
    )]

    # 获取模型响应
    response = spark.generate([messages])

    # 提取并返回回答内容
    if response.generations and len(response.generations) > 0:
        return response.generations[0][0].text
    return "抱歉，未能获取到有效的回答"


questions = [
    "我想知道长沙今天天气怎么样,请你以下列格式回答我：天气,最高气温,最低气温,相对湿度(%)，请严格按照格式回答，如：晴,34,25,88,其他的一个字不要加，包括长沙今天的天气状况为，天气只能是晴雨雾阴中的一种,如果是其他天气，请选择晴雨雾阴中最接近的一种输出，今天好像是阴",
    "我想知道今天的农历日期，直接告诉我日期，不要其他的东西，如：七月廿二"
]

# 测试示例
if __name__ == '__main__':
    question = "我想知道长沙今天天气怎么样,请你以下列格式回答我：天气,最高气温,最低气温,相对湿度(%)，请严格按照格式回答，如：阴,34,25,88,其他的一个字不要加，包括长沙今天的天气状况为，天气只能是晴雨雾阴中的一种,如果是其他天气，请选择晴雨雾阴中最接近的一种输出，今天好像是阴"
    answer = get_llm_response(question)
    print(f"问题: {question}")
    print(f"回答: {answer}")
