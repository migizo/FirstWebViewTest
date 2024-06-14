/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// ディレクトリを指定(末尾に'/'を付ける想定)
juce::String webResourceDir = "index.htmlが配置されているResourceディレクトリ/";

// ローカルサーバー(例: vscode拡張のLive Server等)を使用する場合は'1'に、JUCEのgetResourceProviderRoot()を使用する場合は'0'に指定
//   ただし'1'に指定しても、chromeなどのブラウザ上ではJUCEと接続されず通信できないため注意。(対応するには何かしら別途対策する必要あり)
// ここでのローカルサーバーを使用するメリットとしてはホットリロード(frontend関連のファイルを編集した場合に即座に変更が反映される)が使用できる点で
//   getResourceProviderRoot()でのアクセスの場合は途中で変更しても変更が反映されないようになっている。
#define USE_LOCAL_HOST 0

#if USE_LOCAL_HOST
juce::String urlAddress = "http://127.0.0.1:5500/"; // 'localhost'で指定しても失敗するので'127.0.0.1'とする必要あり
#endif

//==============================================================================
FirstWebViewTestAudioProcessorEditor::FirstWebViewTestAudioProcessorEditor (FirstWebViewTestAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    webComp.reset(new juce::WebBrowserComponent(createWebOptions()));
    addAndMakeVisible(webComp.get());

#if USE_LOCAL_HOST
    // 開発時など既に起動しているローカルサーバーへアクセス
    webComp->goToURL(urlAddress);
#else
    // リリース時などに使用し、JUCEで提供されているResourceProviderのルートにアクセスする
    // このResourceProviderとは軽量サーバーのようなもので
    // このサンプルではgetResource()で内容を定義し、Options::withResourceProviderに渡して使用可能になっている
    webComp->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    addAndMakeVisible(emitButton);
    emitButton.setButtonText("emit 'clickOnBackend'");
    emitButton.addListener(this);
    
    addAndMakeVisible(funcButton);
    funcButton.setButtonText("call 'evaluateJavascript()'");
    funcButton.addListener(this);
    
    addAndMakeVisible(listenerLabel);
    addAndMakeVisible(nativeFuncLabel);

    setSize (400, 300);
}

FirstWebViewTestAudioProcessorEditor::~FirstWebViewTestAudioProcessorEditor()
{
}

//==============================================================================
void FirstWebViewTestAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void FirstWebViewTestAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    nativeFuncLabel.setBounds(area.removeFromBottom(32));
    listenerLabel.setBounds(area.removeFromBottom(32));
    auto btnArea = area.removeFromBottom(32);
    emitButton.setBounds(btnArea.removeFromLeft(btnArea.getWidth() / 2));
    funcButton.setBounds(btnArea);
    webComp->setBounds(area);
}

//==============================================================================
void FirstWebViewTestAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    // frontend上でlisten可能な'clickOnBackend'イベントを発行する
    if (button == &emitButton)
    {
        static int counter = 0;
        webComp->emitEventIfBrowserIsVisible("clickOnBackend", {++counter});
    }
    // frontendコンテキスト上で処理を実行し、返ってきた結果を元にbackend側で処理を行う
    else if (button == &funcButton)
    {
        // 処理完了後,結果を元に処理を行うコールバック
        auto evaluationCallback = [this](juce::WebBrowserComponent::EvaluationResult result) -> void
        {
            // スクリプト評価失敗時は以下のような挙動になりwin/androidでは失敗時の確認が厳しい。(ここでのエラー判定はmacでの確認用)
            // mac,iOS......getError()=構文エラーなど確認可能。getResult()=null
            // win,android...getError()=nullを返すので確認不可。getResult()=有効なvarを返してしまう。
            if (const juce::WebBrowserComponent::EvaluationResult::Error* err = result.getError())
            {
                DBG("error: " << (int)err->type << ", " << err->message);
            }
            if (const juce::var* res = result.getResult())
            {
                int count = res->toString().getTrailingIntValue();
                funcButton.setButtonText("call 'evaluateJavascript()': " + juce::String(count));
            }
        };
        static int counter = 0;
        const auto script = "document.getElementById('changedByBackendText').textContent = 'called by backend: ' + " + juce::String(++counter);
        webComp->evaluateJavascript(script, evaluationCallback);
    }
}

//==============================================================================
juce::WebBrowserComponent::Options FirstWebViewTestAudioProcessorEditor::createWebOptions()
{
    // frontend上で呼び出せる処理
    auto nativeFunction = [this](const juce::Array<juce::var>& argArray,
                             std::function<void(juce::var)> completionForPromise) -> void
    {
        int v = (int)argArray[0] + 1;
        nativeFuncLabel.setText("called by frontend: " + juce::String(v), juce::dontSendNotification);
        completionForPromise(v);
    };
    // frontend上で発行されたイベントに対応して行う処理(=listenerCallback)
    auto nativeEventListener = [this](juce::var v) -> void
    {
        listenerLabel.setText("listen 'clickOnFrontend': " + v.toString(), juce::dontSendNotification);
    };
    // リソース提供処理
    auto resourceProvider = [this](const juce::String& url) { return getResource(url); };
    
    auto options = juce::WebBrowserComponent::Options()
        .withKeepPageLoadedWhenBrowserIsHidden()                    // hiddenでも表示し続けるか(or ブランクページにするか)
        .withNativeIntegrationEnabled()                             // webとc++で通信できるようにするか
        .withNativeFunction("nativeFunc", nativeFunction)           // web側から呼び出せる関数処理
        .withEventListener("clickOnFrontend", nativeEventListener)  // web側での任意イベント発火に対して呼ぶ処理
        .withUserScript("")                                         // リソース読み込み前にwindow.__JUCE__.backendを使用可能な任意のスクリプトを指定
        .withInitialisationData("h1", juce::var("1stWebViewTest"))  // window.__JUCE__.initialisationDataに指定できる変数名と値
#if USE_LOCAL_HOST
        // 第2引数のallowedOriginInについて、
        // ローカルホストでのデバッグ時にfrontend上でResourceProviderの提供するデータを使用したい場合に指定する。
        // これによりローカルホストのfrontend上でjuce.getBackendResourceAddress(path)で取得することが可能
        // (このサンプルでUSE_LOCAL_HOST=1でjuce::URL { urlAddress }.getOrigin()を指定しなくすると現在時間情報のJSONを取得できなくなる)
        .withResourceProvider(resourceProvider, juce::URL { urlAddress }.getOrigin());
#else
        .withResourceProvider(resourceProvider);
#endif
    ;

    return options;
}

// ResourceProviderの処理内容。urlに対応するリソースを返す。
std::optional<juce::WebBrowserComponent::Resource> FirstWebViewTestAudioProcessorEditor::getResource(const juce::String& url)
{
    const auto urlToRetrive = url == "/" ? "index.html"
                                         : url.fromFirstOccurrenceOf ("/", false, false);
    if (urlToRetrive == "index.html")
    {
        return ResourceFromFile(juce::File(webResourceDir + urlToRetrive));
    }
#if ! USE_LOCAL_HOST
    // goToURL(juce::WebBrowserComponent::getResourceProviderRoot())でアクセスしている場合に、
    // スクリプトでimportする必要があるので,ここで設定しておく必要がある。
    // check_native_interop.jsは同階層index.jsからimportするため同じく設定しておく必要がある。
    // 同様にcssやその他のファイルが増えた場合にもここで対応する必要がある
    else if (urlToRetrive == "juce-framework-frontend/javascript/index.js" ||
             urlToRetrive == "juce-framework-frontend/javascript/check_native_interop.js")
    {
        return ResourceFromFile(juce::File(webResourceDir + urlToRetrive));
    }
#endif
    // 動作確認用のため現在時間情報をfrontend上でリソースとして取得できるようにしている。
    else if (urlToRetrive == "infoResource.json")
    {
        juce::DynamicObject::Ptr d (new juce::DynamicObject());
        d->setProperty ("updateTime", juce::Time::getCurrentTime().toString(false, true));
        const auto s = juce::JSON::toString (d.get());
        juce::MemoryInputStream stream { s.getCharPointer(), s.getNumBytesAsUTF8(), false };
        return juce::WebBrowserComponent::Resource{streamToBytes(stream), "application/json"};
    }
    else
    {
        DBG("resourceProvider access with " << urlToRetrive << " is failed.");
    }
    
    return std::nullopt ;
}

// inputStreamをstd::vector<std::byte>に変換
std::vector<std::byte> FirstWebViewTestAudioProcessorEditor::streamToBytes(juce::InputStream& stream)
{
    std::vector<std::byte> result ((size_t) stream.getTotalLength());
    stream.setPosition (0);
    [[maybe_unused]] const auto bytesRead = stream.read (result.data(), result.size());
    jassert (bytesRead == (ssize_t) result.size());
    return result;
}

// juce::Fileのデータをbyte配列化し、拡張子からmimeTypeを判別しResouce型として返す
juce::WebBrowserComponent::Resource FirstWebViewTestAudioProcessorEditor::ResourceFromFile(const juce::File& file)
{
    juce::FileInputStream fis(file);
    std::vector<std::byte> result = streamToBytes(fis);
    
    juce::String extType;
    if (file.hasFileExtension("html")) extType = "text/html";
    else if (file.hasFileExtension("js")) extType = "text/javascript";
    return { result, extType };
}
