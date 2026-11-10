# Exact continuation state

The user authorised finishing the ENTIRE book, private GitHub repo, public Vercel site, fresh writer for every page, independent full technical review for every page, and immediate Vercel publication AFTER EACH FINISHED PAGE. Do not ask permission again to continue the authorised scope.

Project: `/Users/hermes/Desktop/hardware-from-software`
GitHub: https://github.com/EthanHosier/hardware-from-software (private)
Public reading domain: https://hardware-from-software.vercel.app

Read AUTHORING_GUIDE.md, PLAN.md, EXAMPLES.md, catalog.json, WORK_LOG.md, reviews/authoring-status.json, verification.json and releases.json. All53catalog pages remain the agreed scope:44main+9appendices. Mainchapter0 has6complete independently accepted pages. The other47pages are pending; nothing after0.6 should be advertised as authored.

The coordinator `/root/book_coordinator` dispatched dedicated fresh-context writers. Its turn and the next1.1writer hit account usage limit (tool reported reset11Nov2026 00:31). This is an external account limit, not a user-requested pause or cancellation. Coordinator may be resumed; writer1.1 may resume SAME page if it has useful context. New pages require new fork_turns:none writers. No alternative model was used to evade the limit.

Root completed independent review of0.5 and0.6 after their fresh writers finished. Durable reports record source checks, SVG visual checks, hash and physical-test limits. Earlier0.1–0.4 reviews came from coordinator. Published pages must have `status: accepted_source_review`, `canonical_sha256` matching exactMarkdown, `included_source_sha256` for all sources, plus asset hashes for diagram acceptance. Backfill asset hashes from final reviewed assets where absent. Never freeze new content before independent review.

Root owns ALL examples, config, catalog, scripts, Git and Vercel; writers own only their page/uniqueSVGs/report. All examples already implemented. All12Pico targets compile with pinned localofficial tools; all7Linux apps compile nativeArm64 inGitHubActions; both kernel modules MODPOST+link against officialPi6.12.96+rpt-rpi-v8 headers; DToverlay compiles. Host normal+ASan/UBSan parser and10PTY serial cases passed. Linuxmemorylab passed controlled childfault. Inspect verification.json for exactCIcommit; do not rerun unchanged checks merely for ceremony. New edits need affected checks and renewed source-review hashes.

Use the checkout-local scriptedtoolchain as main route; VSCode officialextension optional. This is an intentional refinement of the earlier plan so the instructions match the verified environment. `.deps` contains SDK2.3.1 at079c6f39023649b154152db30f1d781e884879bc, ArmGNU14.2.Rel1, CMake3.31.6/Ninja1.11.1.4, RaspberryPiOpenOCD/picotool tools releasev2.3.1-0. `python3 scripts/setup_deps.py` reproduces localsetup; `python3 scripts/build_examples.py` buildsPico+hosttests. See actual code and scripts for paths.

Public publication:
1. Full independent review/repair/recheck and report hash for ONE completedpage.
2. `python3 scripts/publish.py` immediately. It builds accepted-only stages, checks rendered links/includes, deploys prebuiltproduction, anonymously compares manifest and every livechapter, and appends releases.json.
3. Root commits/pushes coherent changes, including evidence. Git-triggered Verceldeployment disabled in vercel.json, because hosted build does not have mdBook. GitHubActions independently runs sourcechecks when examples change.
4. Do not publish drafts. Build defaults to requiring ALL53acceptedpages, so partialpublishing explicitly uses --partial internally. Unpublishedforwardlinks go toroadmap. Rootactualsrc is preserved; rewrittenlinks exist only in ignored staging.

No hardware connection is verified. Never invent flashing, SWD, GPIO levels, button behaviour, bus traces, sensor results or kernel runtime. Compilechecks and hostedArm64runner do not count as realPi runtime. Keep this distinction in lessons, verificationpage and finalreport.

Existing LearnCpp repo and its Vercel protection stay untouched. New Vercelproject has ssoProtection:null/passwordProtection:null, anonymouslycheckedproduction. `.env.local`createdbyCLI is ignored; never print/copy it. Dependencies, buildoutputs and credentials stayignored. Do not create anotherrepo/project or duplicate the deploymentidentity.

Continue from page 1.1 through appendix F in catalog order: 53 planned pages, 6 complete and 47 pending. Finish independentreviews, checks, progressivepublication and finalglobalverification; do not report wholebookcomplete early.
