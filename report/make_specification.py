"""Regenerate the one-page report: python report/make_specification.py.

Requires ReportLab only for report generation; the C++ application has no
third-party dependencies.
"""
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer


destination = Path(__file__).resolve().parent / 'specification.pdf'
doc = SimpleDocTemplate(str(destination), pagesize=A4, rightMargin=42, leftMargin=42,
                        topMargin=34, bottomMargin=32,
                        title='SignalLab - ITT 036 Specification', author='SignalLab project')
body = ParagraphStyle('Body', fontName='Helvetica', fontSize=9.2, leading=12.2,
                      textColor=colors.HexColor('#233247'), spaceAfter=6, alignment=TA_LEFT)
heading = ParagraphStyle('Section', parent=body, fontName='Helvetica-Bold', fontSize=10.2,
                         textColor=colors.HexColor('#145c88'), spaceBefore=6, spaceAfter=4)
title = ParagraphStyle('Title', parent=body, fontName='Helvetica-Bold', fontSize=21, leading=25)
subtitle = ParagraphStyle('Subtitle', parent=body, fontSize=9.4, textColor=colors.HexColor('#526477'))
code = ParagraphStyle('Code', parent=body, fontName='Courier', fontSize=7.8, leading=10.4)
story = [Paragraph('SignalLab', title),
         Paragraph('ITT 036 Programming Assignment 1 | One-page specification | 20 October 2026', subtitle),
         Spacer(1, 5)]


def section(label, text):
    story.extend([Paragraph(label, heading), Paragraph(text, body)])


section('Objective and implementation',
        'A small C++17 command-line digital signal encoder and decoder. The application uses only '
        'the C++ standard library (including random, vector, cmath, fstream and filesystem). '
        'Existing team modules are retained. SVG graphs are written directly in C++; '
        'no graphics framework or runtime Python dependency is required.')
section('Features and workflow',
        'Select digital or analog input. Digital input accepts typed bits or random streams '
        'with optional guaranteed four/eight-zero runs. Expand-around-center search reports the '
        'longest contiguous palindrome, zero-based position and length; ties choose the earliest '
        '(O(n<super>2</super>) time, O(1) working space). Choose NRZ-L, NRZ-I, Manchester, '
        'Differential Manchester or AMI; AMI offers B8ZS/HDB3 scrambling. After graph export, '
        'optional level-based decoding displays the recovered bits and a round-trip comparison. '
        'Analog input goes through PCM encoding and decoding and can feed any line encoder.')
section('Signal conventions',
        'Normalized levels: +1 V, 0 V, -1 V. NRZ-L: 0=+1, 1=-1. NRZ-I: initial +1, '
        'toggle on 1. Manchester: 0=(+1,-1), 1=(-1,+1), two half-bit levels. Differential '
        'Manchester: initial +1, transition at every midpoint and at the bit boundary for 0. '
        'AMI: first ordinary mark +1, subsequent marks alternate; virtual previous mark -1. '
        'B8ZS substitutes 000VB0VB. HDB3 substitutes 000V after odd ordinary-mark parity '
        'since the last substitution, otherwise B00V; reset parity after substitution. '
        'V repeats the preceding nonzero polarity and B alternates it. Encoding/scrambling '
        'and decoding take O(n) time.')
section('PCM sampling and reconstruction',
        'Manual samples are assumed uniformly sampled at the entered Fs (1-1000000 Hz); '
        'the generated sine uses frequency Fs/8, midpoint offset and half-range amplitude. '
        'For b=1-8 bits, use L=2<super>b</super> uniform intervals and step '
        'D=(maximum-minimum)/L. Encode the interval index MSB first; the maximum endpoint '
        'uses the last interval. Decode to interval midpoints and hold each value for 1/Fs '
        'seconds (zero-order hold). Reject nonfinite/out-of-range samples. Example: range '
        '[-1,1], b=2, samples [-1,0,1] give 00 10 11 and [-0.75,0.25,0.75].')
section('Build, run and output',
        'From the repository root, compile the following as one command; run '
        '<font name="Courier">./signallab</font> (Windows: '
        '<font name="Courier">.\\signallab.exe</font>).')
story.append(Paragraph('g++ -std=c++17 main.cpp data_generator.cpp palindrome.cpp '
                       'line_coding.cpp pcm.cpp waveform.cpp -o signallab', code))
story.append(Paragraph('Open <font name="Courier">output/line_waveform.svg</font> or '
                       '<font name="Courier">output/pcm_staircase.svg</font> in a browser. '
                       'Graphs show at most 64 bits/samples; matching CSVs retain the full data. '
                       'Scrambling graphs include ordinary AMI for comparison. New runs overwrite '
                       'matching output files.', body))
section('Verification and limitations',
        'C++17 compilation, exact voltage/parity examples, seven decoder round trips, generator '
        'and palindrome cases, PCM quantization and timed exports passed focused checks; '
        '26 CLI runs and SVG parsing also passed. Input limits: 10000 bits and 1000 PCM samples. '
        'Decoding analyzes ideal clock-aligned signal levels, not raster images; the stricter '
        'image-analysis extra-credit target is not implemented. No hardware ADC, noise recovery '
        'or analog low-pass reconstruction filter is included. PCM is the selected modulation option.')
section('References',
        'ITT 036 Programming Assignment 1, 21 September 2026, pp. 1-2; existing SignalLab team '
        'source: github.com/DeepakPatel004/SignalLab. This PDF was prepared using Python ReportLab '
        '(report-generation tooling only).')


def footer(canvas, document):
    canvas.setStrokeColor(colors.HexColor('#d9e1e9'))
    canvas.line(42, 30, A4[0] - 42, 30)
    canvas.setFont('Helvetica', 8)
    canvas.setFillColor(colors.HexColor('#526477'))
    canvas.drawString(42, 18, 'SignalLab | Code and specification report are separate submission files.')
    canvas.drawRightString(A4[0] - 42, 18, str(document.page))


doc.build(story, onFirstPage=footer, onLaterPages=footer)
print(destination)
