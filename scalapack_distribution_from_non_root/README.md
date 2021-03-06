# scalapack\_distribution\_from\_non\_root

Array Descriptor の `RSRC_` および `CSRC_` にSourceにしたいプロセスの位置を指定するだけ．

## 実行例

4x4 プロセスグリッド上の (3,3) のプロセスで 16x16 の行列を生成し，
ブロック(2,2)で4x4プロセスに分散する．

```
% mpirun -np 16 ./distribution_from_non_root 16 4 4 2 2 3 3
(3,3): Generating matrix A...
       0      16      32      48      64      80      96     112     128     144     160     176     192     208     224     240
       1      17      33      49      65      81      97     113     129     145     161     177     193     209     225     241
       2      18      34      50      66      82      98     114     130     146     162     178     194     210     226     242
       3      19      35      51      67      83      99     115     131     147     163     179     195     211     227     243
       4      20      36      52      68      84     100     116     132     148     164     180     196     212     228     244
       5      21      37      53      69      85     101     117     133     149     165     181     197     213     229     245
       6      22      38      54      70      86     102     118     134     150     166     182     198     214     230     246
       7      23      39      55      71      87     103     119     135     151     167     183     199     215     231     247
       8      24      40      56      72      88     104     120     136     152     168     184     200     216     232     248
       9      25      41      57      73      89     105     121     137     153     169     185     201     217     233     249
      10      26      42      58      74      90     106     122     138     154     170     186     202     218     234     250
      11      27      43      59      75      91     107     123     139     155     171     187     203     219     235     251
      12      28      44      60      76      92     108     124     140     156     172     188     204     220     236     252
      13      29      45      61      77      93     109     125     141     157     173     189     205     221     237     253
      14      30      46      62      78      94     110     126     142     158     174     190     206     222     238     254
      15      31      47      63      79      95     111     127     143     159     175     191     207     223     239     255

distribute

(0,0)
       0      16     128     144
       1      17     129     145
       8      24     136     152
       9      25     137     153

(0,1)
      32      48     160     176
      33      49     161     177
      40      56     168     184
      41      57     169     185

(0,2)
      64      80     192     208
      65      81     193     209
      72      88     200     216
      73      89     201     217

(0,3)
      96     112     224     240
      97     113     225     241
     104     120     232     248
     105     121     233     249

(1,0)
       2      18     130     146
       3      19     131     147
      10      26     138     154
      11      27     139     155

(1,1)
      34      50     162     178
      35      51     163     179
      42      58     170     186
      43      59     171     187

(1,2)
      66      82     194     210
      67      83     195     211
      74      90     202     218
      75      91     203     219

(1,3)
      98     114     226     242
      99     115     227     243
     106     122     234     250
     107     123     235     251

(2,0)
       4      20     132     148
       5      21     133     149
      12      28     140     156
      13      29     141     157

(2,1)
      36      52     164     180
      37      53     165     181
      44      60     172     188
      45      61     173     189

(2,2)
      68      84     196     212
      69      85     197     213
      76      92     204     220
      77      93     205     221

(2,3)
     100     116     228     244
     101     117     229     245
     108     124     236     252
     109     125     237     253

(3,0)
       6      22     134     150
       7      23     135     151
      14      30     142     158
      15      31     143     159

(3,1)
      38      54     166     182
      39      55     167     183
      46      62     174     190
      47      63     175     191

(3,2)
      70      86     198     214
      71      87     199     215
      78      94     206     222
      79      95     207     223

(3,3)
     102     118     230     246
     103     119     231     247
     110     126     238     254
     111     127     239     255

release context

```
