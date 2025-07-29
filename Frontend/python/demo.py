#  demo.py
#  Ein einfaches Skript, um eine zeitverzögerte Textausgabe zu simulieren,
#  ähnlich der Ausgabe eines ASR-Prozesses. Nützlich für UI-Tests.

import time

text = """[0.00s --> 1.00s] UNKNOWN:  Musik
[1.00s --> 25.98s] UNKNOWN:  Also mein Problem ist normalerweise eher, mich selbst zu begrenzen,
[25.98s --> 30.08s] SPEAKER_00:  als nun zu sagen, mir fällt nichts mehr ein.
[30.22s --> 33.88s] SPEAKER_00:  Das war schon immer so ein Problem, dass ich eher Angst habe,
[33.96s --> 36.92s] SPEAKER_00:  dass ich gar nicht mehr alle Projekte in meinem Leben schaffen kann,
[37.00s --> 41.06s] SPEAKER_00:  die ich noch in meiner großen Ideenschublade rumliegen habe.
[42.44s --> 47.86s] SPEAKER_00:  Das ist tatsächlich so, dass Horrors für mich durch das Thema
[47.86s --> 53.00s] SPEAKER_00:  und durch diese etwas andere Machart
[53.00s --> 54.86s] SPEAKER_00:  gar nicht diese...
[55.98s --> 65.24s] SPEAKER_00:  diese ausschweifende und vielleicht auch grenzenlose Zeit...
[66.10s --> 67.98s] SPEAKER_00:  Ich hatte keine große Erzählung.
[68.72s --> 72.70s] SPEAKER_00:  Das war bei Endlich, dem Vorgängeralbum, war das eben ganz anders,
[73.22s --> 75.10s] SPEAKER_00:  weil ich von vornherein wusste,
[75.98s --> 79.80s] SPEAKER_00:  naja, das Album wird so lang, wie die Geschichte eben braucht,
[79.92s --> 81.12s] SPEAKER_00:  um erzählt zu werden.
[81.60s --> 84.46s] SPEAKER_00:  Und jetzt mit Horrors war es so,
[84.56s --> 85.96s] SPEAKER_00:  dass ich einfach auch zu mir selbst,
[85.98s --> 89.60s] UNKNOWN:  sagen konnte, nein, das muss jetzt gar nicht sein.
[89.78s --> 91.48s] SPEAKER_00:  Vielleicht ist es auch ganz angenehm,
[91.66s --> 93.08s] SPEAKER_00:  mal wieder für den Hörer,
[93.52s --> 96.12s] SPEAKER_00:  wenn man so ein Album mal kurz und knackig macht.
[97.04s --> 100.88s] SPEAKER_00:  Die Aufmerksamkeitsspannen sinken ja sowieso zusehends.
[102.34s --> 103.90s] UNKNOWN:  Aber auch das war kein Grund.
[104.08s --> 106.42s] SPEAKER_00:  Ich hatte das Gefühl, so jetzt ist das eine runde Sache.
[107.14s --> 110.22s] SPEAKER_00:  Die Facetten, die ich erzählen wollte,
[110.64s --> 111.76s] SPEAKER_00:  kommen alle vor.
[111.76s --> 115.76s] SPEAKER_00:  Und da muss man das nicht noch irgendwie künstlich aufblasen,
[115.98s --> 117.98s] SPEAKER_00:  und was natürlich auch sehr gut ist,
[117.98s --> 123.98s] UNKNOWN:  dass wir ohne weiteres mal ein Album auch wieder so machen können,
[123.98s --> 126.98s] SPEAKER_00:  wie zum Beispiel die ersten beiden Alben,
[126.98s --> 132.98s] SPEAKER_00:  dass jetzt passt Horrors einigermaßen auch auf so eine Vinyl wieder drauf,
[132.98s --> 137.98s] SPEAKER_00:  wo man sich nicht jetzt die Haare raufend überlegen muss,
[137.98s --> 139.98s] SPEAKER_00:  was machen wir mit der Spielzeit.
[139.98s --> 142.98s] SPEAKER_00:  Denn viele Leute wollen heute Vinyl hören,
[142.98s --> 144.98s] SPEAKER_00:  aber es gibt natürlich die eine,
[144.98s --> 145.82s] SPEAKER_00:  die wir heute nicht hören wollen,
[145.82s --> 147.82s] SPEAKER_00:  aber es gibt auch die eine Sache,
[147.82s --> 149.82s] SPEAKER_00:  die bei der CD immer schon besser war.
[149.82s --> 151.82s] UNKNOWN:  Es passt einfach mehr drauf.
[151.82s --> 155.82s] SPEAKER_00:  Und das passt jetzt alles ganz gut weitestgehend.
[155.82s --> 158.82s] SPEAKER_00:  Und tatsächlich war es aber eine konzeptionelle Sache,
[158.82s --> 160.82s] SPEAKER_00:  die mir auch Spaß gemacht hat,
[160.82s --> 163.82s] SPEAKER_00:  dass man mit diesen Kurzgeschichten
[163.82s --> 165.82s] SPEAKER_00:  sehr schnell auf den Punkt kommen kann.
[165.82s --> 167.82s] SPEAKER_00:  Das heißt nicht, dass die Stücke alle kurz sind,
[167.82s --> 171.82s] SPEAKER_00:  sondern dass man einfach erzählerisch ganz andere Möglichkeiten wieder hat,
[171.82s --> 173.82s] SPEAKER_00:  als das jetzt bei Endlich der Fall war,
[173.82s --> 174.82s] SPEAKER_00:  wo man einfach gesagt hat,
[174.82s --> 176.82s] SPEAKER_00:  okay, hier habe ich das Gefühl,
[176.82s --> 179.82s] SPEAKER_00:  es fehlt immer noch ein Stück, um die Sache rund zu machen.
[179.82s --> 183.82s] SPEAKER_00:  Dann haben wir noch mal fünf Minuten bis zehn Minuten mehr Spielzeit.
[183.82s --> 185.82s] SPEAKER_00:  Das war jetzt einfach nicht nötig.
[185.82s --> 188.82s] SPEAKER_00:  Eingefallen ist mir relativ viel, leider wieder zu viel.
[188.82s --> 193.82s] SPEAKER_00:  Ich denke, es könnte ohne Weiteres noch ein Horrors 2 oder Horrors 3 geben,
[193.82s --> 198.82s] SPEAKER_00:  aber alles so im Rahmen der Zeit.
[198.82s --> 200.82s] SPEAKER_00:  Und das kann ja immer noch kommen.
[200.82s --> 203.82s] SPEAKER_00:  Also es kann ja auch gerne vielleicht mal wieder,
[203.82s --> 206.82s] SPEAKER_00:  früher ein neues ASP-Album kommen.
[206.82s --> 208.82s] SPEAKER_00:  Und ich habe ja noch ein paar andere Dinge zu tun,
[208.82s --> 210.82s] SPEAKER_00:  wie zum Beispiel mein Heromore-Projekt.
[210.82s --> 217.82s] SPEAKER_00:  Dann wartet noch ein anderes großes Projekt auf mich und die Hörer.
[217.82s --> 221.82s] UNKNOWN:  Und ich denke, dann ist das jetzt eine gute Portion gewesen.
[233.82s --> 237.82s] SPEAKER_00:  Aber die Welt ist sicher schrecklich genug.
[237.82s --> 241.82s] UNKNOWN:  Und vielleicht, wenn ich gewusst hätte,
[241.82s --> 244.82s] SPEAKER_00:  was jetzt noch alles passieren wird,
[244.82s --> 249.82s] UNKNOWN:  hätte ich dann doch ein anderes Thema gewählt.
[249.82s --> 253.82s] SPEAKER_00:  Aber dass wir jetzt in einer Situation leben,
[253.82s --> 257.82s] SPEAKER_00:  in der auch viele Menschen irgendwie,
[257.82s --> 261.82s] SPEAKER_00:  naja, vielleicht eher mit negativen Gefühlen zu tun haben,
[261.82s --> 262.82s] SPEAKER_00:  Ängste haben,
[262.82s --> 265.82s] SPEAKER_00:  wie das so weitergeht mit einem Krieg,
[265.82s --> 269.82s] SPEAKER_00:  der zum ersten Mal nicht am anderen Ende der Welt stattfindet,
[269.82s --> 273.82s] SPEAKER_00:  was die Sache mit den Ängsten auch wieder deutlich realistischer macht,
[273.82s --> 275.82s] SPEAKER_00:  wenn man sie durchdenkt.
[275.82s --> 280.82s] SPEAKER_00:  Aber für viele Leute ist das jetzt näher.
[280.82s --> 282.82s] SPEAKER_00:  Viele machen sich Sorgen,
[282.82s --> 284.82s] SPEAKER_00:  so auch im ganz privaten Bereich, denke ich,
[284.82s --> 286.82s] SPEAKER_00:  wie sie klarkommen,
[286.82s --> 290.82s] SPEAKER_00:  wie sie ihre nächste Gasrechnung bezahlen sollen,
[290.82s --> 291.82s] SPEAKER_00:  wie das alles weitergeht.
[291.82s --> 293.82s] SPEAKER_00:  Und hätte ich gewusst,
[293.82s --> 295.82s] SPEAKER_00:  dass das alles so kommt,
[295.82s --> 297.82s] SPEAKER_00:  was man ja nie weiß,
[297.82s --> 300.82s] SPEAKER_00:  dann hätte ich mir das vielleicht auch erspart.
[300.82s --> 303.82s] SPEAKER_00:  Weil natürlich scheint es so ein bisschen wie das falsche Thema
[303.82s --> 305.82s] SPEAKER_00:  oder zumindest im falschen Moment.
[305.82s --> 307.82s] SPEAKER_00:  Denn ich merke schon,
[307.82s --> 310.82s] SPEAKER_00:  dass wenn die Leute auf Konzerte gehen,
[310.82s --> 312.82s] SPEAKER_00:  wenn sie Musik hören,
[312.82s --> 316.82s] UNKNOWN:  tendieren sie im Moment so ein bisschen eher zum Leichteren,
[316.82s --> 318.82s] SPEAKER_00:  vielleicht auch zu Dingen,
[318.82s --> 320.82s] SPEAKER_00:  mit denen man sich nicht auf so interessiert,
[320.82s --> 323.82s] SPEAKER_00:  sich nicht auf so intensive Weise beschäftigen muss
[323.82s --> 326.82s] SPEAKER_00:  und schon gar nicht mit Gruselthemen.
[326.82s --> 328.82s] SPEAKER_00:  Aber es ist nun mal so, wie es ist.
[328.82s --> 331.82s] SPEAKER_00:  Vielleicht ist es auch gerade dieses antizyklische Verhalten,
[331.82s --> 333.82s] SPEAKER_00:  das dann nachher gut kommt.
[333.82s --> 335.82s] SPEAKER_00:  Ich kann es jetzt sowieso nicht mehr ändern.
[335.82s --> 340.82s] SPEAKER_00:  Ich kann auch nicht was anderes jetzt mal eben aus der Ideenkiste zaubern,
[340.82s --> 345.82s] UNKNOWN:  das dann jetzt unmittelbar erscheinen könnte.
[345.82s --> 347.82s] SPEAKER_00:  Und ich denke aber auch,
[347.82s --> 349.82s] SPEAKER_00:  wir haben wie bei allen Asp-Alben,
[350.82s --> 354.82s] SPEAKER_00:  genug augenzwinkernde Momente drin.
[354.82s --> 356.82s] SPEAKER_00:  Wir haben das, was wir immer haben
[356.82s --> 359.82s] SPEAKER_00:  oder was vielleicht auch ich als Stärke empfinde,
[359.82s --> 363.82s] SPEAKER_00:  dass es trotz allem auch genießbar ist.
[363.82s --> 365.82s] SPEAKER_00:  Es bleibt harmonisch.
[365.82s --> 368.82s] SPEAKER_00:  Wir machen jetzt nicht auf einmal komplett andere Musik,
[368.82s --> 371.82s] SPEAKER_00:  nur weil wir in einem Horror-Thema arbeiten,
[371.82s --> 376.82s] SPEAKER_00:  gibt es jetzt nicht nur verstörende Momente auf dem Album,
[376.82s --> 378.82s] SPEAKER_00:  wo man dann lieber abschalten möchte.
[378.82s --> 380.82s] SPEAKER_00:  Ich glaube, das hat alles,
[380.82s --> 383.82s] SPEAKER_00:  was der Asp-Fan gerne mag.
[383.82s --> 385.82s] SPEAKER_00:  Es hat vor allem alles, was ich gerne mag
[385.82s --> 387.82s] SPEAKER_00:  und sogar noch ein bisschen mehr.
[387.82s --> 390.82s] SPEAKER_00:  Und deswegen denke ich, man kann das schon machen.
[390.82s --> 393.82s] UNKNOWN:  Und eigentlich war ja in allen Alben
[393.82s --> 396.82s] SPEAKER_00:  immer auch ein Gruselthema vorhanden.
[396.82s --> 400.82s] SPEAKER_00:  Und jetzt ist es eben sehr teilweise plakativ,
[400.82s --> 403.82s] SPEAKER_00:  teilweise versteckt, teilweise rätselhaft.
[403.82s --> 408.82s] SPEAKER_00:  Also es hat alle Facetten, die man gerne hat, glaube ich.
[408.82s --> 411.82s] UNKNOWN:  Und ein bisschen gruseln wollen wir uns.
[411.82s --> 413.82s] UNKNOWN:  Eigentlich alle.
[423.82s --> 428.82s] SPEAKER_00:  Ich hatte das Gefühl, besonders darauf hinweisen zu müssen,
[428.82s --> 431.82s] SPEAKER_00:  dass es sich um Kurzgeschichten handelt.
[431.82s --> 435.82s] SPEAKER_00:  Also Asp ist ja seit jeher ein Projekt,
[435.82s --> 437.82s] SPEAKER_00:  das ganz viel mit Literatur,
[437.82s --> 439.82s] SPEAKER_00:  mit Literatur zu tun hat.
[439.82s --> 441.82s] SPEAKER_00:  Ich kann es auch nicht genug betonen,
[441.82s --> 446.82s] SPEAKER_00:  dass unser kleines Genre Gothic Novel Rock
[446.82s --> 450.82s] SPEAKER_00:  eigentlich nicht aus Gothic Rock kommt,
[450.82s --> 452.82s] SPEAKER_00:  sondern eben aus Gothic Novel.
[452.82s --> 455.82s] SPEAKER_00:  Und das ist die klassische Schauer-Geschichte.
[455.82s --> 464.82s] UNKNOWN:  Und dieses Thema wollte ich nach all den unglaublich komplexen Erzählungen
[464.82s --> 466.82s] SPEAKER_00:  wollte ich darauf hinweisen,
[466.82s --> 471.82s] SPEAKER_00:  dass es jetzt sozusagen um eine musikalische Kurzgeschichte geht
[471.82s --> 473.82s] SPEAKER_00:  oder um mehrere Kurzgeschichten.
[473.82s --> 478.82s] SPEAKER_00:  Und dass es zwar durchaus ein Konzeptalbum bleibt,
[478.82s --> 482.82s] SPEAKER_00:  weil wir haben das übergeordnete Thema Horrors,
[482.82s --> 485.82s] SPEAKER_00:  also die Schrecken.
[485.82s --> 488.82s] SPEAKER_00:  Und das verbindet natürlich die Songs.
[488.82s --> 492.82s] SPEAKER_00:  Aber ich wollte, dass jeder einzelne Track
[492.82s --> 494.82s] SPEAKER_00:  eine eigene Kurzgeschichte darstellt,
[494.82s --> 497.82s] SPEAKER_00:  eine musikalische Kurzgeschichte
[497.82s --> 499.82s] SPEAKER_00:  und ein eigenes Thema hat
[499.82s --> 502.82s] SPEAKER_00:  und nicht wie bei den letzten Alben
[502.82s --> 506.82s] SPEAKER_00:  ein großes erzählerisches Konzept,
[506.82s --> 513.82s] UNKNOWN:  das auch sehr starre erzählerische Momente hat,
[513.82s --> 517.82s] SPEAKER_00:  an die man sich als Autor dann einfach halten muss.
[517.82s --> 520.82s] SPEAKER_00:  Und für mich war das wie nach so vielen Jahren
[520.82s --> 522.82s] SPEAKER_00:  ein Muskel loslassen
[522.82s --> 526.82s] UNKNOWN:  und mal einen anderen Muskel trainieren.
[526.82s --> 531.82s] UNKNOWN:  Und das Thema Kurzgeschichte hat mich seit vielen, vielen Jahren interessiert.
[531.82s --> 538.82s] UNKNOWN:  Und dafür den englischen Begriff Novellas gewählt,
[538.82s --> 542.82s] UNKNOWN:  um das nachher direkt auf dem Cover auch zu zeigen.
[552.82s --> 554.82s] UNKNOWN:  Und ich hoffe, dass das Video euch gefallen hat.
[554.82s --> 556.82s] UNKNOWN:  Bis zum nächsten Mal.
[556.82s --> 557.82s] UNKNOWN:  Tschüss."""

#  Gibt den Text zeilenweise mit einer kleinen Pause aus, um Streaming zu simulieren.
for line in text.split('\n'):
    print(line)
    time.sleep(0.5)  # 0.5 Sekunden Pause zwischen den Zeilen
