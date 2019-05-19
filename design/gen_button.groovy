/**
 * The purpose of this script is to generate the text buttons programmatically. It generates an html file
 * representing the button at scale 1 and another html file at scale 2. It then invokes Chrome to render the
 * html and export it as an image. The file button-style.css applies the right styling to the html.
 *
 * Important note: when running this script on a HiDPI screen (4K), Chrome will actually render in high resolution
 *                 which means all images will be 2x what they should be.
 *                 In order to fix this issue, from the Finder, select the Chrome application and right click
 *                 to select "Get Info", and then select the "Open on low resolution" checkbox before running this
 *                 script.
 *
 * Invoking:
 * cd <folder where this script lives>
 * groovy ./gen_button.groovy
 */

import groovy.xml.MarkupBuilder

def actionFont = "Impact"
def actionFontSize = 12

def buttonFont = "Helvetica"
def buttonFontSize = 13

// actions (waveform)
[
  "action_clear": "Clear",
  "action_crop": "Crop",
  "action_cut": "Cut",
  "action_match": "Match",
  "action_norm_0dB": "0 dB",
  "action_norm_3dB": "-3 dB",
  "action_norm_6dB": "-6 dB",
  "action_redo": "Redo",
  "action_trim": "Trim",
  "action_undo": "Undo",
  "action_zoom": "Zoom",
  ].each { name, title ->
  button(name, title, 40, 18, actionFont, actionFontSize, true)
}

// IO
button("action_load", "Load", 40, 18, actionFont, actionFontSize, false)
button("action_export", "Export", 40, 18, actionFont, actionFontSize, true)

// Sample
button("action_sample", "Sample", 400, 18, actionFont, actionFontSize, true)
button("sample_input_In1", "In 1", 26, 20, buttonFont, buttonFontSize, false)
button("sample_input_In2", "In 2", 26, 20, buttonFont, buttonFontSize, false)

// Slices
[1,2,4,8,16,32,48,64].each { slice ->
  button("slice${slice < 10 ? '0' + slice : slice}", slice.toString(), 18, 18, buttonFont, buttonFontSize, false)
}

// Banks
["A", "B", "C", "D"].each { bank ->
  button("bank${bank}", bank, 18, 18, buttonFont, buttonFontSize, false)
}

// Plain button
button("button", "&nbsp;", 15, 15, buttonFont, buttonFontSize, false)

// Tabs
button("play", "Play", 70, 25, actionFont, 18, false)
button("edit", "Edit", 70, 25, actionFont, 18, false)

// Handle
button("slider_handle", "&nbsp;", 22, 15, buttonFont, buttonFontSize, false)

/**
 * generates a button in 1x and 2x resolutions
 */
static def button(String buttonName,
                  String buttonTitle,
                  int width,
                  int height,
                  String fontFamily,
                  int fontSize,
                  boolean includeDisableState)
{
  def resourceDir = new File("../resource")

  [1: "${buttonName}", 2: "${buttonName}_2x"].each { scale, name ->

    def buttonHtmlFile = new File("${name}.html")

    buttonHtmlFile.withWriter { writer ->
      genButtonHtml(writer, buttonName, buttonTitle, width, height, fontFamily, fontSize, scale, includeDisableState)
    }

    def imageWidth = width + 8
    def imageHeight = (height + 8) * (includeDisableState ? 3 : 2)

    imageWidth *= scale
    imageHeight *= scale

    def command = ["/Applications/Google Chrome.app/Contents/MacOS/Google Chrome", "--headless", "--screenshot", "--window-size=${imageWidth},${imageHeight}", "--default-background-color=0", buttonHtmlFile.path]

    println command

    def out = new StringBuffer()
    def err = new StringBuffer()
    def proc = command.execute()
    proc.waitForProcessOutput(out, err)
    if(proc.exitValue() != 0)
      throw new RuntimeException("""Error while executing command:
--- Output ---
${out}
--- Error ---
${err}
""")
    new AntBuilder().move(file: "screenshot.png", tofile: new File(resourceDir, "${name}.png"))
    buttonHtmlFile.delete()
  }
}

static def genButtonHtml(def writer,
                         String buttonName,
                         String buttonTitle,
                         int width,
                         int height,
                         String fontFamily,
                         int fontSize,
                         int scale,
                         boolean includeDisableState)
{
  writer << "<!DOCTYPE html>\n"

  def html_style = """
      .scale-1x .button {
        font-family: '${fontFamily}';
        font-size: ${fontSize}px;
        width: ${width}px;
        height: ${height}px;
      }
      .scale-2x .button {
        font-family: '${fontFamily}';
        font-size: ${fontSize * 2}px;
        width: ${width * 2}px;
        height: ${height * 2}px;
      }
  """

  def xml = new MarkupBuilder(writer)
  xml.html {
   head {
    meta(charset: "UTF-8")
    title(buttonName)
    link(rel: "stylesheet", href:"button-style.css")
    style(html_style)
   }
   body {
     table('class': "scale-${scale}x") {
       if(includeDisableState) {
       tr {
         td {
           div('class': "button-frame button-disabled") {
             div('class': "button specular-light-off") {
               div('class': "title") {
                 mkp.yieldUnescaped(buttonTitle)
               }
             }
           }
         }
       }
       }
       tr {
         td {
           div('class': "button-frame button-off") {
             div('class': "button specular-light-on") {
               div('class': "title") {
                 mkp.yieldUnescaped(buttonTitle)
               }
             }
           }
         }
       }
       tr {
         td {
           div('class': "button-frame button-on") {
             div('class': "button specular-light-off") {
               div('class': "title") {
                 mkp.yieldUnescaped(buttonTitle)
               }
             }
           }
         }
       }
     }
   }
  }

  writer.close()

  return writer.toString()
}

