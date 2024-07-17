; ModuleID = '/home/LLVM_MYSQL/knob_profiler/baseline/example.cc'
source_filename = "/home/LLVM_MYSQL/knob_profiler/baseline/example.cc"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [31 x i8] c"Number of iterations = %d|%lf\0A\00", align 1
@.str.1 = private unnamed_addr constant [18 x i8] c"iteration No. %d\0A\00", align 1

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local noundef i32 @_Z5func0v() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  store i32 0, i32* %2, align 4
  store i32 0, i32* %3, align 4
  store i32 10, i32* %4, align 4
  br label %5

5:                                                ; preds = %21, %0
  %6 = load i32, i32* %2, align 4
  %7 = icmp slt i32 %6, 100
  br i1 %7, label %8, label %11

8:                                                ; preds = %5
  %9 = load i32, i32* %3, align 4
  %10 = icmp slt i32 %9, 100000
  br label %11

11:                                               ; preds = %8, %5
  %12 = phi i1 [ false, %5 ], [ %10, %8 ]
  br i1 %12, label %13, label %28

13:                                               ; preds = %11
  %14 = load i32, i32* %2, align 4
  %15 = load i32, i32* %3, align 4
  %16 = add nsw i32 %14, %15
  %17 = load i32, i32* %4, align 4
  %18 = add nsw i32 %16, %17
  %19 = load i32, i32* %1, align 4
  %20 = add nsw i32 %19, %18
  store i32 %20, i32* %1, align 4
  br label %21

21:                                               ; preds = %13
  %22 = load i32, i32* %2, align 4
  %23 = add nsw i32 %22, 1
  store i32 %23, i32* %2, align 4
  %24 = load i32, i32* %3, align 4
  %25 = add nsw i32 %24, 1
  store i32 %25, i32* %3, align 4
  %26 = load i32, i32* %4, align 4
  %27 = add nsw i32 %26, -1
  store i32 %27, i32* %4, align 4
  br label %5, !llvm.loop !4

28:                                               ; preds = %11
  %29 = load i32, i32* %1, align 4
  ret i32 %29
}

; Function Attrs: mustprogress noinline optnone uwtable
define dso_local noundef i32 @_Z5func1v() #1 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  store i32 0, i32* %2, align 4
  br label %3

3:                                                ; preds = %7, %0
  %4 = load i32, i32* %1, align 4
  %5 = add nsw i32 %4, 1
  store i32 %5, i32* %1, align 4
  %6 = icmp slt i32 %4, 1000
  br i1 %6, label %7, label %12

7:                                                ; preds = %3
  %8 = load i32, i32* %1, align 4
  %9 = load i32, i32* %2, align 4
  %10 = add nsw i32 %9, %8
  store i32 %10, i32* %2, align 4
  %11 = call noundef i32 @_Z5func3v()
  br label %3, !llvm.loop !6

12:                                               ; preds = %3
  %13 = load i32, i32* %2, align 4
  ret i32 %13
}

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local noundef i32 @_Z5func3v() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  store i32 0, i32* %2, align 4
  br label %3

3:                                                ; preds = %7, %0
  %4 = load i32, i32* %1, align 4
  %5 = add nsw i32 %4, 1
  store i32 %5, i32* %1, align 4
  %6 = icmp slt i32 %4, 1000
  br i1 %6, label %7, label %11

7:                                                ; preds = %3
  %8 = load i32, i32* %1, align 4
  %9 = load i32, i32* %2, align 4
  %10 = add nsw i32 %9, %8
  store i32 %10, i32* %2, align 4
  br label %3, !llvm.loop !7

11:                                               ; preds = %3
  %12 = load i32, i32* %2, align 4
  ret i32 %12
}

; Function Attrs: mustprogress noinline optnone uwtable
define dso_local noundef i32 @_Z5func2b(i1 noundef zeroext %0) #1 {
  %2 = alloca i32, align 4
  %3 = alloca i8, align 1
  %4 = alloca i8, align 1
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = zext i1 %0 to i8
  store i8 %9, i8* %3, align 1
  %10 = load i8, i8* %3, align 1
  %11 = trunc i8 %10 to i1
  %12 = zext i1 %11 to i8
  store i8 %12, i8* %4, align 1
  %13 = load i8, i8* %4, align 1
  %14 = trunc i8 %13 to i1
  br i1 %14, label %15, label %27

15:                                               ; preds = %1
  store i32 0, i32* %5, align 4
  store i32 0, i32* %6, align 4
  br label %16

16:                                               ; preds = %20, %15
  %17 = load i32, i32* %5, align 4
  %18 = add nsw i32 %17, 1
  store i32 %18, i32* %5, align 4
  %19 = icmp slt i32 %17, 1000
  br i1 %19, label %20, label %25

20:                                               ; preds = %16
  %21 = load i32, i32* %5, align 4
  %22 = load i32, i32* %6, align 4
  %23 = add nsw i32 %22, %21
  store i32 %23, i32* %6, align 4
  %24 = call noundef i32 @_Z5func3v()
  br label %16, !llvm.loop !8

25:                                               ; preds = %16
  %26 = load i32, i32* %6, align 4
  store i32 %26, i32* %2, align 4
  br label %38

27:                                               ; preds = %1
  store i32 0, i32* %7, align 4
  store i32 0, i32* %8, align 4
  br label %28

28:                                               ; preds = %32, %27
  %29 = load i32, i32* %7, align 4
  %30 = add nsw i32 %29, 1
  store i32 %30, i32* %7, align 4
  %31 = icmp slt i32 %29, 1000
  br i1 %31, label %32, label %36

32:                                               ; preds = %28
  %33 = load i32, i32* %7, align 4
  %34 = load i32, i32* %8, align 4
  %35 = add nsw i32 %34, %33
  store i32 %35, i32* %8, align 4
  br label %28, !llvm.loop !9

36:                                               ; preds = %28
  %37 = load i32, i32* %8, align 4
  store i32 %37, i32* %2, align 4
  br label %38

38:                                               ; preds = %36, %25
  %39 = load i32, i32* %2, align 4
  ret i32 %39
}

; Function Attrs: mustprogress noinline optnone uwtable
define dso_local void @_Z5func4i(i32 noundef %0) #1 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca double, align 8
  store i32 %0, i32* %2, align 4
  store i32 1000, i32* %3, align 4
  %5 = load i32, i32* %2, align 4
  %6 = sitofp i32 %5 to double
  %7 = fadd double %6, 1.200000e+00
  store double %7, double* %4, align 8
  %8 = load i32, i32* %3, align 4
  %9 = load double, double* %4, align 8
  %10 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([31 x i8], [31 x i8]* @.str, i64 0, i64 0), i32 noundef %8, double noundef %9)
  br label %11

11:                                               ; preds = %31, %1
  %12 = load i32, i32* %3, align 4
  %13 = add nsw i32 %12, -1
  store i32 %13, i32* %3, align 4
  %14 = icmp ne i32 %12, 0
  br i1 %14, label %15, label %32

15:                                               ; preds = %11
  %16 = load i32, i32* %3, align 4
  %17 = srem i32 %16, 100
  %18 = icmp eq i32 %17, 0
  br i1 %18, label %19, label %23

19:                                               ; preds = %15
  %20 = load i32, i32* %3, align 4
  %21 = sub nsw i32 1000, %20
  %22 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([18 x i8], [18 x i8]* @.str.1, i64 0, i64 0), i32 noundef %21)
  br label %23

23:                                               ; preds = %19, %15
  %24 = call noundef i32 @_Z5func1v()
  %25 = load double, double* %4, align 8
  %26 = fcmp ogt double %25, 5.000000e+00
  br i1 %26, label %27, label %29

27:                                               ; preds = %23
  %28 = call noundef i32 @_Z5func2b(i1 noundef zeroext false)
  br label %31

29:                                               ; preds = %23
  %30 = call noundef i32 @_Z5func2b(i1 noundef zeroext true)
  br label %31

31:                                               ; preds = %29, %27
  br label %11, !llvm.loop !10

32:                                               ; preds = %11
  ret void
}

declare dso_local i32 @printf(i8* noundef, ...) #2

; Function Attrs: mustprogress noinline norecurse optnone uwtable
define dso_local noundef i32 @main(i32 noundef %0, i8** noundef %1) #3 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8**, align 8
  %6 = alloca i32, align 4
  %7 = alloca i8, align 1
  store i32 0, i32* %3, align 4
  store i32 %0, i32* %4, align 4
  store i8** %1, i8*** %5, align 8
  %8 = load i8**, i8*** %5, align 8
  %9 = getelementptr inbounds i8*, i8** %8, i64 1
  %10 = load i8*, i8** %9, align 8
  %11 = call i64 @atoll(i8* noundef %10) #5
  %12 = trunc i64 %11 to i32
  store i32 %12, i32* %6, align 4
  store i8 1, i8* %7, align 1
  %13 = load i32, i32* %6, align 4
  call void @_Z5func4i(i32 noundef %13)
  %14 = load i8, i8* %7, align 1
  %15 = trunc i8 %14 to i1
  %16 = zext i1 %15 to i32
  call void @_Z5func4i(i32 noundef %16)
  ret i32 0
}

; Function Attrs: nounwind readonly willreturn
declare dso_local i64 @atoll(i8* noundef) #4

attributes #0 = { mustprogress noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress noinline optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { mustprogress noinline norecurse optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind readonly willreturn "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { nounwind readonly willreturn }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 1}
!2 = !{i32 7, !"frame-pointer", i32 2}
!3 = !{!"clang version 14.0.6 (https://github.com/llvm/llvm-project.git f28c006a5895fc0e329fe15fead81e37457cb1d1)"}
!4 = distinct !{!4, !5}
!5 = !{!"llvm.loop.mustprogress"}
!6 = distinct !{!6, !5}
!7 = distinct !{!7, !5}
!8 = distinct !{!8, !5}
!9 = distinct !{!9, !5}
!10 = distinct !{!10, !5}
