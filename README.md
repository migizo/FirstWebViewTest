# FirstWebViewTest
simple valnila js example with JUCE8 webview 

# 概要
出来るだけシンプルな構成でJUCE8のjuce::WebBrowserComponentでUIを構築するためのサンプルプログラムです。
簡略化のため以下のようになっています。
- 素のjavascript(=vanilaJS)を用いてwebフロントエンド関連のファイル構成をシンプルにしています。
- juce::AudioProcessorとjuce::AudioProcessorEditorのそれぞれの継承クラスのみで、JUCE側は主にPluginEditor.cppファイルのコメントを見れば挙動を確認できるようにしています。
- juce::WebSliderRelayやWebSliderParameterAttachmentなどのパラメータ関連は省略しています。

# 手順
クローンした後、以下を行ってください
- Resource/juce-framework-frontendディレクトリにJUCE/modules/juce_gui_extra/native/javascriptフォルダを複製して配置してください。
- PluginEditor.cpp冒頭のwebResourceDir変数に(index.htmlが配置されている)Resourceディレクトリを指定してください。
- 状況に応じて同じくPluginEditor.cpp冒頭のUSE_LOCAL_HOSTフラグやurlAddress変数を変更してください。
